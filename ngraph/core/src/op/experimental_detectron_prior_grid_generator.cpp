//*****************************************************************************
// Copyright 2017-2020 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//*****************************************************************************

#include <memory>

#include "itt.hpp"
#include "ngraph/attribute_visitor.hpp"
#include "ngraph/op/experimental_detectron_prior_grid_generator.hpp"
#include "ngraph/runtime/host_tensor.hpp"

using namespace std;
using namespace ngraph;

NGRAPH_RTTI_DEFINITION(op::v6::ExperimentalDetectronPriorGridGenerator,
                       "ExperimentalDetectronPriorGridGenerator",
                       6);

op::v6::ExperimentalDetectronPriorGridGenerator::ExperimentalDetectronPriorGridGenerator(
    const Output<Node>& priors,
    const Output<Node>& im_data,
    const Output<Node>& feature_map,
    const Attributes& attrs)
    : Op({priors, feature_map, im_data})
    , m_attrs(attrs)
{
    constructor_validate_and_infer_types();
}

bool op::v6::ExperimentalDetectronPriorGridGenerator::visit_attributes(AttributeVisitor& visitor)
{
    visitor.on_attribute("flatten", m_attrs.flatten);
    visitor.on_attribute("h", m_attrs.h);
    visitor.on_attribute("w", m_attrs.w);
    visitor.on_attribute("stride_x", m_attrs.stride_x);
    visitor.on_attribute("stride_y", m_attrs.stride_y);
    return true;
}

shared_ptr<Node> op::v6::ExperimentalDetectronPriorGridGenerator::clone_with_new_inputs(
    const OutputVector& new_args) const
{
    check_new_args_count(this, new_args);

    return make_shared<op::v6::ExperimentalDetectronPriorGridGenerator>(
        new_args.at(0), new_args.at(1), new_args.at(2), m_attrs);
}

static constexpr size_t priors_port = 0;
static constexpr size_t featmap_port = 1;
static constexpr size_t im_data_port = 2;

void op::v6::ExperimentalDetectronPriorGridGenerator::validate()
{
    auto priors_shape = get_input_partial_shape(priors_port);
    auto featmap_shape = get_input_partial_shape(featmap_port);
    auto im_data_shape = get_input_partial_shape(im_data_port);

    if (priors_shape.rank().is_dynamic() || featmap_shape.rank().is_dynamic())
    {
        return;
    }

    NODE_VALIDATION_CHECK(
        this, priors_shape.rank().get_length() == 2, "Priors rank must be equal to 2.");

    NODE_VALIDATION_CHECK(this,
                          priors_shape[1].is_static() && priors_shape[1].get_length() == 4u,
                          "The last dimension of the 'priors' input must be equal to 4. Got: ",
                          priors_shape[1]);

    NODE_VALIDATION_CHECK(
        this, featmap_shape.rank().get_length() == 4, "Feature_map rank must be equal to 4.");

    if (im_data_shape.rank().is_dynamic())
    {
        return;
    }

    NODE_VALIDATION_CHECK(
        this, im_data_shape.rank().get_length() == 4, "Im_data rank must be equal to 4.");

    const auto num_batches_featmap = featmap_shape[0];
    const auto num_batches_im_data = im_data_shape[0];
    NODE_VALIDATION_CHECK(this,
                          num_batches_featmap.same_scheme(num_batches_im_data),
                          "The first dimension of both 'feature_map' and 'im_data' must match. "
                          "Feature_map: ",
                          num_batches_featmap,
                          "; Im_data: ",
                          num_batches_im_data);
}

void op::v6::ExperimentalDetectronPriorGridGenerator::validate_and_infer_types()
{
    auto priors_shape = get_input_partial_shape(priors_port);
    auto featmap_shape = get_input_partial_shape(featmap_port);
    auto input_et = get_input_element_type(0);

    validate();

    PartialShape out_shape;
    set_output_size(1);
    if (priors_shape.rank().is_dynamic() || featmap_shape.rank().is_dynamic())
    {
        out_shape = {Dimension::dynamic(), Dimension::dynamic(), Dimension::dynamic(), 4};
        if (m_attrs.flatten)
        {
            out_shape = PartialShape{Dimension::dynamic(), 4};
        }

        set_output_type(0, input_et, out_shape);
        return;
    }

    const size_t priors_num = static_cast<size_t>(priors_shape[0].get_length());
    const size_t grid_height = static_cast<size_t>(featmap_shape[2].get_length());
    const size_t grid_width = static_cast<size_t>(featmap_shape[3].get_length());

    if (m_attrs.flatten)
    {
        out_shape = Shape{grid_height * grid_width * priors_num, 4};
    }
    else
    {
        out_shape = Shape{grid_height, grid_width, priors_num, 4};
    }
    set_output_type(0, input_et, out_shape);
}