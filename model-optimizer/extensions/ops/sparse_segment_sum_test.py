"""
 Copyright (C) 2018-2021 Intel Corporation

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

import unittest

import numpy as np

from extensions.ops.sparse_segment_sum import SparseSegmentSum
from mo.front.common.partial_infer.utils import int64_array
from mo.graph.graph import Node
from mo.utils.unittest.graph import build_graph

# graph 1
nodes_attributes1 = {'input_data': {'shape': None, 'value': None, 'kind': 'data'},
                    'input_indices': {'shape': None, 'value': None, 'kind': 'data'},
                    'input_segment_ids': {'shape': None, 'value': None, 'kind': 'data'},
                    'sparse_segment_sum_node': {'op': 'SparseSegmentSum', 'kind': 'op'},
                    'output_segments': {'shape': None, 'value': None, 'kind': 'data'},
                    }

edges1 = [('input_data', 'sparse_segment_sum_node', {'in': 0}),
          ('input_indices', 'sparse_segment_sum_node', {'in': 1}),
          ('input_segment_ids', 'sparse_segment_sum_node', {'in': 2}),
          ('sparse_segment_sum_node', 'output_segments', {'out': 0})]

inputs1 = {'input_data': {'shape': int64_array([20, 4, 5]), 'value': None},
           'input_indices': {'shape': int64_array([40]), 'value': None},
           'input_segment_ids': {'shape': int64_array([40]), 'value': None}}

# graph 2 with constant input, sum
nodes_attributes2 = {'input_data': {'shape': None, 'value': None, 'kind': 'data'},
                    'input_indices': {'shape': None, 'value': None, 'kind': 'data'},
                    'input_segment_ids': {'shape': None, 'value': None, 'kind': 'data'},
                    'sparse_segment_sum_node': {'op': 'SparseSegmentSum', 'kind': 'op'},
                    'output_segments': {'shape': None, 'value': None, 'kind': 'data'},
                    }

edges2 = [('input_data', 'sparse_segment_sum_node', {'in': 0}),
          ('input_indices', 'sparse_segment_sum_node', {'in': 1}),
          ('input_segment_ids', 'sparse_segment_sum_node', {'in': 2}),
          ('sparse_segment_sum_node', 'output_segments', {'out': 0})]

inputs2 = {'input_data': {'shape': int64_array([3, 4]), 'value': np.array([[1, 2, 3, 4], [-1, -2, -3, -4], [5, 6, 7, 8]], dtype=np.float)},
           'input_indices': {'shape': int64_array([3]), 'value': np.array([0, 1, 2], dtype=np.float)},
           'input_segment_ids': {'shape': int64_array([3]), 'value': np.array([0, 0, 1], dtype=np.float)}}

class TestSparseSegmentSum(unittest.TestCase):
    def test_partial_infer(self):
        graph = build_graph(nodes_attributes1, edges1, inputs1)

        sparse_segment_sum_node = Node(graph, 'sparse_segment_sum_node')
        SparseSegmentSum.infer(sparse_segment_sum_node)

        # prepare reference results
        ref_output_segments_shape = int64_array([40, 4, 5])

        # get resulted shapes
        res_output_segments_shape = graph.node['output_segments']['shape']

        self.assertTrue(np.array_equal(ref_output_segments_shape, res_output_segments_shape),
                        'Shapes do not match expected: {} and given: {}'.format(ref_output_segments_shape, res_output_segments_shape))

    def test_incorrect_shapes(self):
        inputs = {'input_data': {'shape': int64_array([20, 4, 5]), 'value': None},
                   'input_indices': {'shape': int64_array([39]), 'value': None},
                   'input_segment_ids': {'shape': int64_array([40]), 'value': None}}
        graph = build_graph(nodes_attributes1, edges1, inputs)
        sparse_segment_sum_node = Node(graph, 'sparse_segment_sum_node')
        self.assertRaises(AssertionError, SparseSegmentSum.infer, sparse_segment_sum_node)

    def test_infer_constant_input_sum(self):
        graph = build_graph(nodes_attributes2, edges2, inputs2)

        sparse_segment_sum_node = Node(graph, 'sparse_segment_sum_node')
        SparseSegmentSum.infer(sparse_segment_sum_node)

        # prepare reference results
        ref_output_segments_shape = int64_array([2, 4])
        ref_output_segments_value = np.array([[0, 0, 0, 0], [5, 6, 7, 8]], dtype=np.float)

        # get resulted shapes
        res_output_segments_shape = graph.node['output_segments']['shape']
        res_output_segments_value = graph.node['output_segments']['value']

        self.assertTrue(np.array_equal(ref_output_segments_shape, res_output_segments_shape),
                        'Shapes do not match expected: {} and given: {}'.format(ref_output_segments_shape, res_output_segments_shape))
        self.assertTrue(np.array_equal(ref_output_segments_value, res_output_segments_value),
                        'Shapes do not match expected: {} and given: {}'.format(ref_output_segments_value, res_output_segments_value))
