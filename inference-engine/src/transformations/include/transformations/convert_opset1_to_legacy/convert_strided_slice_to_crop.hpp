// Copyright (C) 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <vector>
#include <memory>
#include <algorithm>

#include <transformations_visibility.hpp>

#include <ngraph/pass/graph_rewrite.hpp>

namespace ngraph {
namespace pass {

class TRANSFORMATIONS_API ConvertStridedSliceToCropMatcher;

}  // namespace pass
}  // namespace ngraph

class ngraph::pass::ConvertStridedSliceToCropMatcher: public ngraph::pass::MatcherPass {
public:
    ConvertStridedSliceToCropMatcher();
};
