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

from mo.front.common.partial_infer.elemental import copy_shape_infer
from mo.graph.graph import Node, Graph
from mo.ops.op import Op


class ConvertLike(Op):
    op = 'ConvertLike'
    enabled = False

    def __init__(self, graph: Graph, attrs: dict):
        mandatory_props = {
            'op': self.op,
            'type': self.op,
            'version': 'opset1',

            'infer': copy_shape_infer,
            'type_infer': self.type_infer,

            'in_ports_count': 2,
            'out_ports_count': 1,
        }
        super().__init__(graph, mandatory_props, attrs)

    @staticmethod
    def type_infer(node: Node):
        assert node.is_in_port_connected(1), 'The second input is not connected for a node {}.' \
                                             ''.format(node.soft_get('name'), node.id)
        node.out_port(0).set_data_type(node.in_port(1).get_data_type())
