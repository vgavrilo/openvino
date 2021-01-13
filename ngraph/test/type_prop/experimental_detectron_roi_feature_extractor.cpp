//*****************************************************************************
// Copyright 2020 Intel Corporation
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

#include <vector>

#include "gtest/gtest.h"
#include "ngraph/ngraph.hpp"
#include "util/type_prop.hpp"

using namespace ngraph;

using Attrs = op::v6::ExperimentalDetectronROIFeatureExtractor::Attributes;
using ExperimentalROI = op::v6::ExperimentalDetectronROIFeatureExtractor;

TEST(type_prop, detectron_roi_feature_extractor)
{
    Attrs attrs;
    attrs.aligned = false;
    attrs.output_size = 14;
    attrs.preserve_rois_order = 1;
    attrs.sampling_ratio = 2;
    attrs.pyramid_scales = {4, 8, 16, 32};
    attrs.image_id = 0;

    auto input = std::make_shared<op::Parameter>(element::f32, Shape{1000, 4});
    auto pyramid_layer0 = std::make_shared<op::Parameter>(element::f32, Shape{1, 256, 200, 336});
    auto pyramid_layer1 = std::make_shared<op::Parameter>(element::f32, Shape{1, 256, 100, 168});
    auto pyramid_layer2 = std::make_shared<op::Parameter>(element::f32, Shape{1, 256, 50, 84});
    auto pyramid_layer3 = std::make_shared<op::Parameter>(element::f32, Shape{1, 256, 25, 42});

    auto roi = std::make_shared<ExperimentalROI>(
        NodeVector{input, pyramid_layer0, pyramid_layer1, pyramid_layer2, pyramid_layer3}, attrs);

    ASSERT_EQ(roi->get_output_element_type(0), element::f32);
    EXPECT_EQ(roi->get_output_shape(0), (Shape{1000, 256, 14, 14}));
}

TEST(type_prop, detectron_roi_feature_extractor_dynamic)
{
    Attrs attrs;
    attrs.aligned = false;
    attrs.output_size = 14;
    attrs.preserve_rois_order = 1;
    attrs.sampling_ratio = 2;
    attrs.pyramid_scales = {4, 8, 16, 32};
    attrs.image_id = 0;

    struct Shapes
    {
        PartialShape input_shape;
        Dimension channels;
    };

    const auto dyn_dim = Dimension::dynamic();

    std::vector<Shapes> shapes = {
        {{1000, 4}, dyn_dim}, {{dyn_dim, 4}, 256}, {{dyn_dim, 4}, dyn_dim}};
    for (const auto& s : shapes)
    {
        auto layer0_shape = PartialShape{1, s.channels, 200, 336};
        auto layer1_shape = PartialShape{1, s.channels, 100, 168};
        auto layer2_shape = PartialShape{1, s.channels, 50, 84};
        auto layer3_shape = PartialShape{1, s.channels, 25, 42};

        auto ref_out_shape = PartialShape{s.input_shape[0], s.channels, 14, 14};

        auto input = std::make_shared<op::Parameter>(element::f32, s.input_shape);
        auto pyramid_layer0 = std::make_shared<op::Parameter>(element::f32, layer0_shape);
        auto pyramid_layer1 = std::make_shared<op::Parameter>(element::f32, layer1_shape);
        auto pyramid_layer2 = std::make_shared<op::Parameter>(element::f32, layer2_shape);
        auto pyramid_layer3 = std::make_shared<op::Parameter>(element::f32, layer3_shape);

        auto roi = std::make_shared<ExperimentalROI>(
            NodeVector{input, pyramid_layer0, pyramid_layer1, pyramid_layer2, pyramid_layer3},
            attrs);

        ASSERT_EQ(roi->get_output_element_type(0), element::f32);
        ASSERT_TRUE(roi->get_output_partial_shape(0).same_scheme(ref_out_shape));
    }
}
