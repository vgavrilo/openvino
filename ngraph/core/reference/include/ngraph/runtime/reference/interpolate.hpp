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

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include "ngraph/coordinate_transform.hpp"
#include "ngraph/shape_util.hpp"

namespace ngraph
{
    namespace runtime
    {
        namespace reference
        {
            using Nearest_mode = ngraph::op::v4::Interpolate::NearestMode;
            using Transform_mode = ngraph::op::v4::Interpolate::CoordinateTransformMode;
            using InterpolateMode = ngraph::op::v4::Interpolate::InterpolateMode;

            class GetNearestPixel final
            {
            public:
                GetNearestPixel()
                    : GetNearestPixel(Nearest_mode::round_prefer_floor)
                {
                }

                GetNearestPixel(Nearest_mode mode)
                    : m_mode{mode}
                    , m_func{get_func(mode)}
                {
                }

                ~GetNearestPixel() = default;

                int64_t operator()(float original, bool is_downsample) const
                {
                    return m_func(original, is_downsample);
                }

            private:
                using Func = std::function<int64_t(float, bool)>;

                Nearest_mode m_mode;
                Func m_func;

                Func get_func(Nearest_mode mode)
                {
                    switch (mode)
                    {
                    case Nearest_mode::round_prefer_ceil:
                        return [](float x_original, bool) {
                            return static_cast<int64_t>(std::round(x_original));
                        };
                    case Nearest_mode::floor:
                        return [](float x_original, bool) {
                            return static_cast<int64_t>(std::floor(x_original));
                        };
                    case Nearest_mode::ceil:
                        return [](float x_original, bool) {
                            return static_cast<int64_t>(std::ceil(x_original));
                        };
                    case Nearest_mode::simple:
                        return [](float x_original, bool is_downsample) {
                            if (is_downsample)
                            {
                                return static_cast<int64_t>(std::ceil(x_original));
                            }
                            else
                            {
                                return static_cast<int64_t>(x_original);
                            }
                        };
                    default:;
                    }
                    return [](float x_original, bool) {
                        if (x_original == static_cast<int64_t>(x_original) + 0.5f)
                        {
                            return static_cast<int64_t>(std::floor(x_original));
                        }
                        return static_cast<int64_t>(std::round(x_original));
                    };
                }
            };

            class GetOriginalCoordinate final
            {
            public:
                GetOriginalCoordinate()
                    : GetOriginalCoordinate(Transform_mode::half_pixel)
                {
                }

                GetOriginalCoordinate(Transform_mode mode)
                    : m_mode{mode}
                    , m_func{get_func(mode)}
                {
                }

                ~GetOriginalCoordinate() = default;

                float operator()(float x_resized,
                                 float x_scale,
                                 float length_resized,
                                 float length_original) const
                {
                    return m_func(x_resized, x_scale, length_resized, length_original);
                }

            private:
                using Func = std::function<float(float, float, float, float)>;

                Transform_mode m_mode;
                Func m_func;

                Func get_func(Transform_mode mode)
                {
                    switch (mode)
                    {
                    case Transform_mode::pytorch_half_pixel:
                        return [](float x_resized, float x_scale, float length_resized, float) {
                            return length_resized > 1 ? (x_resized + 0.5f) / x_scale - 0.5f : 0.0f;
                        };
                        break;
                    case Transform_mode::asymmetric:
                        return [](float x_resized, float x_scale, float, float) {
                            return x_resized / x_scale;
                        };
                        break;
                    case Transform_mode::tf_half_pixel_for_nn:
                        return [](float x_resized, float x_scale, float, float) {
                            return (x_resized + 0.5f) / x_scale;
                        };
                        break;
                    case Transform_mode::align_corners:
                        return [](
                            float x_resized, float, float length_resized, float length_original) {
                            return length_resized == 1
                                       ? 0
                                       : x_resized * (length_original - 1) / (length_resized - 1);
                        };
                        break;
                    default:;
                    }
                    return [](float x_resized, float x_scale, float, float) {
                        return ((x_resized + 0.5f) / x_scale) - 0.5f;
                    };
                }
            };

            class InterpolateEvalHelper final
            {
            public:
                InterpolateEvalHelper() = default;

                InterpolateEvalHelper(const op::v4::Interpolate::InterpolateAttrs& attrs,
                                      const Shape& input_data_shape,
                                      const std::vector<int64_t>& axes,
                                      const Shape& out_shape,
                                      const std::vector<float>& scales)
                    : m_get_nearest_pixel{attrs.nearest_mode}
                    , m_get_original_coord{attrs.coordinate_transformation_mode}
                    , m_interp_mode{attrs.mode}
                    , m_antialias{attrs.antialias}
                    , m_cube_coeff{attrs.cube_coeff}
                    , m_input_data_shape{input_data_shape}
                    , m_axes{axes}
                    , m_out_shape{out_shape}
                    , m_scales{scales}
                {
                }

                ~InterpolateEvalHelper() = default;

                float triangle_coeff(float dz);
                std::array<float, 4> get_cubic_coeff(float s, float a);
                float get_in_coord(float coord, int64_t axis_idx);

                Coordinate get_input_coords_for_nearest_mode(const Coordinate& output_coord);

                struct InfoForLinearONNXMode
                {
                    std::vector<float> y_original;
                    std::vector<float> x_original;

                    std::vector<int64_t> input_width_mul_y1;
                    std::vector<int64_t> input_width_mul_y2;
                    std::vector<int64_t> in_x1;
                    std::vector<int64_t> in_x2;

                    std::vector<float> dy1;
                    std::vector<float> dy2;
                    std::vector<float> dx1;
                    std::vector<float> dx2;

                    int64_t batch_size;
                    int64_t num_channels;
                    int64_t input_height;
                    int64_t input_width;
                    int64_t output_height;
                    int64_t output_width;
                };

                InfoForLinearONNXMode get_info_for_linear_onnx_mode();

                struct InfoForLinearMode
                {
                    bool antialias;
                    float prod_a;
                    std::vector<float> a;
                    std::vector<int64_t> r;
                    Shape shape_for_indeces;
                };

                InfoForLinearMode get_info_for_linear_mode();

                struct ICoords
                {
                    std::vector<float> icoords;
                    std::vector<int64_t> icoords_r;
                };

                ICoords get_icoords(const Coordinate& output_coord);

                struct LinearModeInnerIterationResult
                {
                    bool condition;
                    float w;
                    Coordinate inner_coord;
                };

                LinearModeInnerIterationResult inner_calculation(const Coordinate& output_coord,
                                                                 const ICoords& icoords_data,
                                                                 const InfoForLinearMode& info,
                                                                 const Coordinate& index);

                int64_t clip_coord(int64_t coord, float length)
                {
                    return std::max(static_cast<int64_t>(0),
                                    std::min(coord, static_cast<int64_t>(length) - 1));
                }

            private:
                GetNearestPixel m_get_nearest_pixel;
                GetOriginalCoordinate m_get_original_coord;
                InterpolateMode m_interp_mode;
                bool m_antialias;
                double m_cube_coeff;

                Shape m_input_data_shape;
                std::vector<int64_t> m_axes;
                Shape m_out_shape;

                std::vector<float> m_scales;
            };

            template <typename T>
            class InterpolateEval final
            {
            public:
                InterpolateEval() = default;

                InterpolateEval(const op::v4::Interpolate::InterpolateAttrs& attrs)
                    : m_attrs{attrs}
                    , m_interp_mode{attrs.mode}
                    , m_cube_coeff{attrs.cube_coeff}
                {
                }

                ~InterpolateEval() = default;

                void operator()(const T* input_data,
                                const Shape& input_data_shape,
                                const std::vector<float>& scales,
                                const std::vector<int64_t>& axes,
                                T* out,
                                const Shape& out_shape)
                {
                    m_input_data_shape = input_data_shape;
                    m_axes = axes;
                    m_out_shape = out_shape;

                    std::size_t output_data_size = shape_size(out_shape);
                    std::fill(out, out + output_data_size, T{});

                    m_scales = scales;

                    helper =
                        InterpolateEvalHelper{m_attrs, input_data_shape, axes, out_shape, scales};

                    switch (m_interp_mode)
                    {
                    case InterpolateMode::nearest: nearest_func(input_data, out); break;
                    case InterpolateMode::linear: linear_func(input_data, out); break;
                    case InterpolateMode::linear_onnx: linear_onnx_func(input_data, out); break;
                    case InterpolateMode::cubic: cubic_func(input_data, out); break;
                    }
                }

            private:
                op::v4::Interpolate::InterpolateAttrs m_attrs;

                InterpolateMode m_interp_mode;
                double m_cube_coeff;

                Shape m_input_data_shape;
                std::vector<int64_t> m_axes;
                Shape m_out_shape;

                std::vector<float> m_scales;

                InterpolateEvalHelper helper;

                void linear_func(const T* input_data, T* out);
                void linear_onnx_func(const T* input_data, T* out);
                void cubic_func(const T* input_data, T* out);
                void nearest_func(const T* input_data, T* out);
            };

            template <typename T>
            void InterpolateEval<T>::linear_func(const T* input_data, T* out)
            {
                std::size_t input_rank = m_input_data_shape.size();
                std::size_t num_of_axes = m_axes.size();

                auto info = helper.get_info_for_linear_mode();

                CoordinateTransform output_transform(m_out_shape);
                CoordinateTransform input_transform(m_input_data_shape);

                for (const Coordinate& output_coord : output_transform)
                {
                    auto icoords_data = helper.get_icoords(output_coord);

                    float summa = 0.0f;
                    float wsum = 0.0f;

                    CoordinateTransform indices{info.shape_for_indeces};
                    for (const auto& index : indices)
                    {
                        auto inner_result =
                            helper.inner_calculation(output_coord, icoords_data, info, index);
                        if (!inner_result.condition)
                        {
                            continue;
                        }

                        wsum += inner_result.w;
                        summa += inner_result.w *
                                 static_cast<float>(
                                     input_data[input_transform.index(inner_result.inner_coord)]);
                    }

                    if (wsum == 0.0f)
                    {
                        out[output_transform.index(output_coord)] = T{};
                    }
                    else
                    {
                        out[output_transform.index(output_coord)] = static_cast<T>(summa / wsum);
                    }
                }
            }

            template <typename T>
            void InterpolateEval<T>::linear_onnx_func(const T* input_data, T* out)
            {
                std::size_t input_rank = m_input_data_shape.size();

                assert((input_rank == 2) || (input_rank == 4));
                assert(m_axes.size() == 2);
                bool correct_axes = ((m_axes[0] == 0) && (m_axes[1] == 1)) ||
                                    ((m_axes[0] == 2) && (m_axes[1] == 3));
                assert(correct_axes);

                const auto info = helper.get_info_for_linear_onnx_mode();

                int64_t batch_size = info.batch_size;
                int64_t num_channels = info.num_channels;
                int64_t output_height = info.output_height;
                int64_t output_width = info.output_width;
                int64_t input_height = info.input_height;
                int64_t input_width = info.input_width;

                const T* xdata = input_data;
                T* ydata = out;
                for (int64_t n = 0; n < batch_size; ++n)
                {
                    for (int64_t c = 0; c < num_channels; ++c)
                    {
                        for (int64_t y = 0; y < output_height; ++y)
                        {
                            for (int64_t x = 0; x < output_width; ++x)
                            {
                                T x11 = xdata[info.input_width_mul_y1[y] + info.in_x1[x]];
                                T x21 = xdata[info.input_width_mul_y1[y] + info.in_x2[x]];
                                T x12 = xdata[info.input_width_mul_y2[y] + info.in_x1[x]];
                                T x22 = xdata[info.input_width_mul_y2[y] + info.in_x2[x]];

                                ydata[output_width * y + x] =
                                    static_cast<T>(info.dx2[x] * info.dy2[y] * x11 +
                                                   info.dx1[x] * info.dy2[y] * x21 +
                                                   info.dx2[x] * info.dy1[y] * x12 +
                                                   info.dx1[x] * info.dy1[y] * x22);
                            }
                        }
                        xdata += input_height * input_width;
                        ydata += output_width * output_height;
                    }
                }
            }

            template <typename T>
            void InterpolateEval<T>::cubic_func(const T* input_data, T* out)
            {
                std::size_t input_rank = m_input_data_shape.size();
                std::size_t num_of_axes = m_axes.size();

                std::cout << "Calculation for the cubic mode\n";
                std::cout << "Axes: ";
                for (const std::size_t axis : m_axes)
                {
                    std::cout << axis << " ";
                }
                std::cout << "\ninput rank: " << input_rank << "\n";
                std::cout << "input shape:  " << m_input_data_shape << "\n";
                std::cout << "output shape: " << m_out_shape << "\n";

                CoordinateTransform output_transform(m_out_shape);
                CoordinateTransform input_transform(m_input_data_shape);
                Shape indices_shape{num_of_axes, 4};

                std::cout << "indices_shape: " << indices_shape << "\n";
                for (const Coordinate& output_coord : output_transform)
                {
                    auto input_coord = output_coord;
                    std::map<std::size_t, std::array<float, 4>> cubic_coeffs;
                    for (std::size_t i = 0; i < num_of_axes; ++i)
                    {
                        int64_t axis = m_axes[i];
                        float coordinate = static_cast<float>(output_coord[axis]);
                        float in_coord = helper.get_in_coord(coordinate, i);
                        int64_t in_coord_int = static_cast<int64_t>(std::floor(in_coord));
                        input_coord[axis] = in_coord_int;
                        auto s = static_cast<float>(in_coord - in_coord_int);
                        cubic_coeffs[axis] = helper.get_cubic_coeff(s, m_cube_coeff);
                    }

                    float summa = 0.0f;
                    CoordinateTransform indices{indices_shape};
                    for (const Coordinate& idx : indices)
                    {
                        auto coords_for_sum = input_coord;
                        float coeffs_prod = 1.0;
                        for (std::size_t i = 0; i < num_of_axes; ++i)
                        {
                            int64_t axis = m_axes[i];
                            coords_for_sum[axis] =
                                helper.clip_coord(input_coord[axis] + idx[i] - 1,
                                                  static_cast<float>(m_input_data_shape[axis]));
                            coeffs_prod *= cubic_coeffs[axis][idx[i]];
                        }
                        summa += coeffs_prod * input_data[input_transform.index(coords_for_sum)];
                    }

                    out[output_transform.index(output_coord)] = static_cast<T>(summa);
                }
            }

            template <typename T>
            void InterpolateEval<T>::nearest_func(const T* input_data, T* out)
            {
                CoordinateTransform output_transform(m_out_shape);
                CoordinateTransform input_transform(m_input_data_shape);

                for (const Coordinate& output_coord : output_transform)
                {
                    auto input_coord = helper.get_input_coords_for_nearest_mode(output_coord);
                    out[output_transform.index(output_coord)] =
                        input_data[input_transform.index(input_coord)];
                }
            }

            template <typename T>
            void interpolate(const T* input_data,
                             const Shape& input_data_shape,
                             const std::vector<float>& scales,
                             const std::vector<int64_t>& axes,
                             T* out,
                             const Shape& out_shape,
                             const op::v4::Interpolate::InterpolateAttrs& attrs)
            {
                InterpolateEval<T> evaluator{attrs};
                evaluator(input_data, input_data_shape, scales, axes, out, out_shape);
            }
        }
    }
}
