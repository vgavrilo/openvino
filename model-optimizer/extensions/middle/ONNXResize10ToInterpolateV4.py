"""
 Copyright (C) 2020 Intel Corporation
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
      http://www.apache.org/licenses/LICENSE-2.0
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
"""

import logging as log
import numpy as np

from extensions.ops.Cast import Cast
from extensions.ops.activation_ops import Floor
from extensions.ops.elementwise import Add, Mul
from extensions.ops.interpolate import Interpolate
from mo.front.common.layout import get_depth_dim, get_height_dim, get_width_dim
from mo.front.common.partial_infer.utils import int64_array, float_array
from mo.front.tf.graph_utils import create_op_with_const_inputs
from mo.graph.graph import Graph, Node, rename_nodes
from mo.middle.passes.convert_data_type import data_type_str_to_np
from mo.middle.replacement import MiddleReplacementPattern
from mo.ops.const import Const
from mo.ops.shape import Shape
from mo.ops.strided_slice import StridedSlice


def replace_resize(graph: Graph, resize: Node):
    log.debug("Converting of ONNX Resize-10 to Interpolate-4 "
              "is triggered for node {}.".format(resize.soft_get('name', resize.id)))

    input_shape = resize.in_port(0).data.get_shape()
    input_rank = len(input_shape)
    resize_name = resize.soft_get('name', resize.id)
    if input_rank not in {4, 5}:
        log.warning('The input shape is not 4D or 5D for op with name {}'.format(resize_name))
        return

    layout = graph.graph['layout']

    if input_rank == 4:
        begin_dim = get_height_dim(layout, input_rank)
        end_dim = get_width_dim(layout, input_rank) + 1
    else:
        begin_dim = get_depth_dim(layout, input_rank)
        end_dim = get_width_dim(layout, input_rank) + 1

    sizes_ss = create_op_with_const_inputs(graph, StridedSlice,
                                           {1: int64_array([begin_dim]),
                                            2: int64_array([end_dim]),
                                            3: int64_array([1])},
                                           {'name': resize_name + '/sizes_ss_',
                                            'begin_mask': int64_array([1]),
                                            'end_mask': int64_array([1]),
                                            'new_axis_mask': int64_array([0]),
                                            'shrink_axis_mask': int64_array([0]),
                                            'ellipsis_mask': int64_array([0])})
    scales_ss = create_op_with_const_inputs(graph, StridedSlice,
                                            {1: int64_array([begin_dim]),
                                             2: int64_array([end_dim]),
                                             3: int64_array([1])},
                                            {'name': resize_name + '/scales_ss_',
                                             'begin_mask': int64_array([1]),
                                             'end_mask': int64_array([1]),
                                             'new_axis_mask': int64_array([0]),
                                             'shrink_axis_mask': int64_array([0]),
                                             'ellipsis_mask': int64_array([0])})
    axes_node = Const(graph,
                      {'name': resize_name + '/axis_',
                       'value': int64_array(np.arange(begin_dim, end_dim))}).create_node()

    interpolate_node = Interpolate(graph, {'version': 'opset4',
                                           'mode': 'linear_onnx' if resize.mode == 'linear' else 'nearest',
                                           'coordinate_transformation_mode': 'asymmetric',
                                           'cube_coeff': -0.75,
                                           'nearest_mode': 'simple',
                                           'pads_begin': int64_array([0]),
                                           'pads_end': int64_array([0]),
                                           'antialias': 0,
                                           'shape_calculation_mode': 'scales',
                                           'in_ports_count': 4}).create_node()

    axes_node.out_port(0).connect(interpolate_node.in_port(3))
    shape_of = Shape(graph, {'name': resize_name + '/ShapeOf_'}).create_node()

    add_node = create_op_with_const_inputs(graph, Add,
                                           {1: float_array([1.0e-5])},
                                           {'name': resize_name + '/Add_'})

    input_data_type = data_type_str_to_np(graph.graph['cmd_params'].data_type)

    cast_shape_to_float = Cast(graph, {'dst_type': input_data_type}).create_node()
    mul_node = Mul(graph, {'name': resize_name + '/Mul_'}).create_node()
    shape_of.out_port(0).connect(cast_shape_to_float.in_port(0))
    cast_shape_to_float.out_port(0).connect(mul_node.in_port(0))
    cast_add_result_to_int = Cast(graph, {'dst_type': np.int64}).create_node()
    floor_node = Floor(graph, {'name': resize_name + '/Floor_'}).create_node()
    mul_node.out_port(0).connect(add_node.in_port(0))
    add_node.out_port(0).connect(floor_node.in_port(0))
    floor_node.out_port(0).connect(cast_add_result_to_int.in_port(0))
    cast_add_result_to_int.out_port(0).connect(sizes_ss.in_port(0))
    sizes_ss.out_port(0).connect(interpolate_node.in_port(1))
    scales_ss.out_port(0).connect(interpolate_node.in_port(2))

    connection_of_resize_input = resize.in_port(0).get_connection()
    connection_of_resize_input.set_destination(interpolate_node.in_port(0))

    connection_of_scales = resize.in_port(1).get_connection()
    connection_of_scales.set_destination(scales_ss.in_port(0))

    connection_of_resize_input.get_source().connect(shape_of.in_port(0))
    connection_of_scales.get_source().connect(mul_node.in_port(1))

    rename_nodes([(resize, resize_name + '/delete'), (interpolate_node, resize_name)])
    resize.out_port(0).get_connection().set_source(interpolate_node.out_port(0))


class ONNXResize10ToInterpolate4(MiddleReplacementPattern):
    """
    The transformation replaces ONNX Resize 10 with Interpolate-4.
    """
    enabled = True

    def run_before(self):
        from extensions.middle.InterpolateSequenceToInterpolate import InterpolateSequenceToInterpolate
        return [InterpolateSequenceToInterpolate]

    def find_and_replace_pattern(self, graph: Graph):
        resize10_ops = graph.get_op_nodes(op='ONNXResize10')
        for resize in resize10_ops:
            replace_resize(graph, resize)
