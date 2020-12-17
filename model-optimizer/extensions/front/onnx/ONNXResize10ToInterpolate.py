"""
 Copyright (C) 2018-2020 Intel Corporation

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

from extensions.ops.activation_ops import Floor
from extensions.ops.Cast import Cast
from extensions.ops.elementwise import Add, Mul
from extensions.ops.interpolate import Interpolate
from extensions.ops.range import Range
from extensions.ops.rank import Rank
from mo.front.common.partial_infer.utils import int64_array, float_array
from mo.front.common.replacement import FrontReplacementOp
from mo.front.tf.graph_utils import create_op_with_const_inputs
from mo.graph.graph import Graph, Node, rename_nodes
from mo.middle.passes.convert_data_type import data_type_str_to_np
from mo.ops.shape import Shape
from mo.ops.strided_slice import StridedSlice


def replace_resize(graph: Graph, resize: Node):
    log.debug("Converting of ONNX Resize-10 to Interpolate-4 "
              "is triggered for node {}.".format(resize.soft_get('name', resize.id)))

    resize_name = resize.soft_get('name', resize.id)

    rank_node = Rank(graph, {'name': resize_name + '/max_axes'}).create_node()
    range_node = create_op_with_const_inputs(graph, Range, {0: int64_array(2), 2: int64_array(1)},
                                             {'name': resize_name + '/axes'})

    sizes_ss = create_op_with_const_inputs(graph, StridedSlice,
                                           {1: int64_array([2]),
                                            2: int64_array([0]),
                                            3: int64_array([1])},
                                           {'name': resize_name + '/sizes_ss',
                                            'begin_mask': int64_array([1]),
                                            'end_mask': int64_array([0]),
                                            'new_axis_mask': int64_array([0]),
                                            'shrink_axis_mask': int64_array([0]),
                                            'ellipsis_mask': int64_array([0])})
    scales_ss = create_op_with_const_inputs(graph, StridedSlice,
                                            {1: int64_array([2]),
                                             2: int64_array([0]),
                                             3: int64_array([1])},
                                            {'name': resize_name + '/scales_ss',
                                             'begin_mask': int64_array([1]),
                                             'end_mask': int64_array([0]),
                                             'new_axis_mask': int64_array([0]),
                                             'shrink_axis_mask': int64_array([0]),
                                             'ellipsis_mask': int64_array([0])})

    rank_node.out_port(0).connect(range_node.in_port(1))

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

    range_node.out_port(0).connect(interpolate_node.in_port(3))
    shape_of = Shape(graph, {'name': resize_name + '/ShapeOf'}).create_node()

    add_node = create_op_with_const_inputs(graph, Add,
                                           {1: float_array([1.0e-5])},
                                           {'name': resize_name + '/Add'})

    input_data_type = data_type_str_to_np(graph.graph['cmd_params'].data_type)

    cast_shape_to_float = Cast(graph, {'dst_type': input_data_type}).create_node()
    mul_node = Mul(graph, {'name': resize_name + '/Mul'}).create_node()
    floor_node = Floor(graph, {'name': resize_name + '/Floor'}).create_node()
    cast_mul_result_to_int = Cast(graph, {'dst_type': np.int64}).create_node()

    shape_of.out_port(0).connect(cast_shape_to_float.in_port(0))
    cast_shape_to_float.out_port(0).connect(mul_node.in_port(0))
    add_node.out_port(0).connect(mul_node.in_port(1))
    mul_node.out_port(0).connect(floor_node.in_port(0))
    floor_node.out_port(0).connect(cast_mul_result_to_int.in_port(0))
    cast_mul_result_to_int.out_port(0).connect(sizes_ss.in_port(0))
    sizes_ss.out_port(0).connect(interpolate_node.in_port(1))

    scales_ss.out_port(0).connect(interpolate_node.in_port(2))

    connection_of_resize_input = resize.in_port(0).get_connection()
    connection_of_resize_input.set_destination(interpolate_node.in_port(0))

    connection_of_scales = resize.in_port(1).get_connection()
    connection_of_scales.set_destination(scales_ss.in_port(0))

    connection_of_resize_input.get_source().connect(shape_of.in_port(0))
    connection_of_resize_input.get_source().connect(rank_node.in_port(0))
    connection_of_scales.get_source().connect(add_node.in_port(0))

    rename_nodes([(resize, resize_name + '/delete'), (interpolate_node, resize_name)])
    resize.out_port(0).get_connection().set_source(interpolate_node.out_port(0))


class ONNXResize10ToInterpolate(FrontReplacementOp):
    """
    The transformation replaces ONNX Resize 10 with Interpolate-4.
    """
    op = 'ONNXResize10'
    enabled = True

    def run_after(self):
        from extensions.front.InterpolateNormalizer import InterpolateNormalizer
        return [InterpolateNormalizer]

    def replace_sub_graph(self, graph: Graph, match: dict):
        resize = match['op']
        replace_resize(graph, resize)
