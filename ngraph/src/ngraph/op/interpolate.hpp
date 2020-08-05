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

#include <cstdint>
#include <vector>
#include "ngraph/attribute_adapter.hpp"
#include "ngraph/op/op.hpp"
#include "ngraph/op/util/attr_types.hpp"

namespace ngraph
{
    namespace op
    {
        namespace v0
        {
            /// \brief Structure that specifies attributes for interpolation
            struct InterpolateAttrs
            {
                // specify dimension indices where interpolation is applied, and `axes` is any
                // unordered list of indeces of different dimensions of input tensor. Required.
                AxisSet axes;
                // specifies type of interpolation
                // one of `nearest`, `linear`, `cubic`, `area`. Required.
                std::string mode;
                // a flag that specifies whether to align corners or not.
                // `true` (default) means the alignment is applied,
                // `false` means the alignment isn't applied.
                bool align_corners = true;
                // a flag that specifies whether to perform anti-aliasing. default is `false`
                bool antialias = false;
                // specify the number of pixels to add to the beginning of the image being
                // interpolated. This addition of pixels is done before interpolation calculation.
                std::vector<size_t> pads_begin;
                // specify the number of pixels to add to the end of the image being interpolated.
                // This addition of pixels is done before interpolation calculation.
                std::vector<size_t> pads_end;
            };

            /// \brief Layer which performs bilinear interpolation
            class NGRAPH_API Interpolate : public Op
            {
            public:
                static constexpr NodeTypeInfo type_info{"Interpolate", 0};
                const NodeTypeInfo& get_type_info() const override { return type_info; }
                enum class InterpolateMode
                {
                    nearest,
                    linear,
                    cubic,
                    area
                };

                Interpolate() = default;
                /// \brief Constructs a Interpolate operation
                ///
                /// \param image        Input image
                /// \param output_shape Output shape of spatial axes
                /// \param attrs        Interpolation attributes
                Interpolate(const Output<Node>& image,
                            const Output<Node>& output_shape,
                            const InterpolateAttrs& attrs);
                bool visit_attributes(AttributeVisitor& visitor) override;

                void validate_and_infer_types() override;

                virtual std::shared_ptr<Node>
                    clone_with_new_inputs(const OutputVector& new_args) const override;

                const InterpolateAttrs& get_attrs() const { return m_attrs; }
            private:
                InterpolateAttrs m_attrs;
            };
        } // namespace v0

        namespace v4
        {
            class NGRAPH_API Interpolate : public Op
            {
            public:
                static constexpr NodeTypeInfo type_info{"Interpolate", 4};
                const NodeTypeInfo& get_type_info() const override { return type_info; }
                enum class InterpolateMode
                {
                    nearest,
                    linear,
                    linear_onnx,
                    cubic
                };

                enum class CoordinateTransformMode
                {
                    half_pixel,
                    pytorch_half_pixel,
                    asymmetric,
                    tf_half_pixel_for_nn,
                    align_corners
                };

                enum class NearestMode
                {
                    round_prefer_floor,
                    round_prefer_ceil,
                    floor,
                    ceil,
                    simple
                };

                struct InterpolateAttrs
                {
                    // specifies type of interpolation
                    // one of `nearest`, `linear`, `linear_onnx`, `cubic` Required.
                    InterpolateMode mode;
                    // specify the number of pixels to add to the beginning of the image being
                    // interpolated. This addition of pixels is done before interpolation
                    // calculation.
                    std::vector<size_t> pads_begin;
                    // specify the number of pixels to add to the end of the image being
                    // interpolated. This addition of pixels is done before interpolation
                    // calculation.
                    std::vector<size_t> pads_end;
                    // specifies how to transform the coordinate in the resized tensor to the
                    // coordinate in the original tensor. one of `half_pixel`, `pytorch_half_pixel`,
                    // `asymmetric`, `tf_half_pixel_for_nn`, `align_corners`
                    CoordinateTransformMode coordinate_transformation_mode;
                    // specifies round mode when `mode == nearest` and is used only when `mode ==
                    // nearest`. one of `round_prefer_floor`, `round_prefer_ceil`, `floor`, `ceil`,
                    // `simple`
                    NearestMode nearest_mode;
                    // a flag that specifies whether to perform anti-aliasing. default is `false`
                    bool antialias;
                    // specifies the parameter *a* for cubic interpolation (see, e.g.
                    // [article](https://ieeexplore.ieee.org/document/1163711/)).  *cube_coeff* is
                    // used only when `mode == cubic`
                    double cube_coeff;

                    InterpolateAttrs()
                        : coordinate_transformation_mode(CoordinateTransformMode::half_pixel)
                        , nearest_mode(NearestMode::round_prefer_floor)
                        , antialias(false)
                        , cube_coeff(-0.75f)
                    {
                    }

                    InterpolateAttrs(InterpolateMode mode,
                                     std::vector<size_t> pads_begin,
                                     std::vector<size_t> pads_end,
                                     CoordinateTransformMode coordinate_transformation_mode =
                                         CoordinateTransformMode::half_pixel,
                                     NearestMode nearest_mode = NearestMode::round_prefer_floor,
                                     bool antialias = false,
                                     double cube_coeff = -0.75)
                        : mode(mode)
                        , pads_begin(pads_begin)
                        , pads_end(pads_end)
                        , coordinate_transformation_mode(coordinate_transformation_mode)
                        , nearest_mode(nearest_mode)
                        , antialias(antialias)
                        , cube_coeff(cube_coeff)
                    {
                    }
                };

                Interpolate() = default;
                /// \brief Constructs a Interpolate operation without 'axes' input.
                ///
                /// \param image  Input image
                /// \param scales Scales of spatial axes, i.e. output_shape / input_shape
                /// \param attrs  Interpolation attributes
                Interpolate(const Output<Node>& image,
                            const Output<Node>& scales,
                            const InterpolateAttrs& attrs);

                /// \brief Constructs a Interpolate operation with 'axes' input.
                ///
                /// \param image  Input image
                /// \param scales Scales of spatial axes, i.e. output_shape / input_shape
                /// \param axes   Interpolation axes
                /// \param attrs  Interpolation attributes
                Interpolate(const Output<Node>& image,
                            const Output<Node>& scales,
                            const Output<Node>& axes,
                            const InterpolateAttrs& attrs);
                bool visit_attributes(AttributeVisitor& visitor) override;

                void validate_and_infer_types() override;

                virtual std::shared_ptr<Node>
                    clone_with_new_inputs(const OutputVector& new_args) const override;
                bool evaluate(const HostTensorVector& outputs,
                              const HostTensorVector& inputs) const override;

                const InterpolateAttrs& get_attrs() const { return m_attrs; }
                std::vector<int64_t> get_axes() const;

            private:
                InterpolateAttrs m_attrs;

                void correct_pads();
            };
        } // namespace v4
        using v0::Interpolate;
        using v0::InterpolateAttrs;
    } // namespace op

    //---------------------------------------- v0 --------------------------------------------------

    template <>
    class NGRAPH_API AttributeAdapter<op::v0::InterpolateAttrs> : public VisitorAdapter
    {
    public:
        AttributeAdapter(op::v0::InterpolateAttrs& ref);

        virtual bool visit_attributes(AttributeVisitor& visitor) override;
        static constexpr DiscreteTypeInfo type_info{"AttributeAdapter<op::v0::InterpolateAttrs>",
                                                    0};
        const DiscreteTypeInfo& get_type_info() const override { return type_info; }
    protected:
        op::v0::InterpolateAttrs& m_ref;
    };

    NGRAPH_API
    std::ostream& operator<<(std::ostream& s, const op::v0::Interpolate::InterpolateMode& type);

    template <>
    class NGRAPH_API AttributeAdapter<op::v0::Interpolate::InterpolateMode>
        : public EnumAttributeAdapterBase<op::v0::Interpolate::InterpolateMode>
    {
    public:
        AttributeAdapter(op::v0::Interpolate::InterpolateMode& value)
            : EnumAttributeAdapterBase<op::v0::Interpolate::InterpolateMode>(value)
        {
        }

        static constexpr DiscreteTypeInfo type_info{
            "AttributeAdapter<op::v0::Interpolate::InterpolateMode>", 0};
        const DiscreteTypeInfo& get_type_info() const override { return type_info; }
    };

    //---------------------------------------- v4 --------------------------------------------------

    NGRAPH_API
    std::ostream& operator<<(std::ostream& s, const op::v4::Interpolate::InterpolateMode& type);

    template <>
    class NGRAPH_API AttributeAdapter<op::v4::Interpolate::InterpolateMode>
        : public EnumAttributeAdapterBase<op::v4::Interpolate::InterpolateMode>
    {
    public:
        AttributeAdapter(op::v4::Interpolate::InterpolateMode& value)
            : EnumAttributeAdapterBase<op::v4::Interpolate::InterpolateMode>(value)
        {
        }

        static constexpr DiscreteTypeInfo type_info{
            "AttributeAdapter<op::v4::Interpolate::InterpolateMode>", 4};
        const DiscreteTypeInfo& get_type_info() const override { return type_info; }
    };

    NGRAPH_API
    std::ostream& operator<<(std::ostream& s,
                             const op::v4::Interpolate::CoordinateTransformMode& type);

    template <>
    class NGRAPH_API AttributeAdapter<op::v4::Interpolate::CoordinateTransformMode>
        : public EnumAttributeAdapterBase<op::v4::Interpolate::CoordinateTransformMode>
    {
    public:
        AttributeAdapter(op::v4::Interpolate::CoordinateTransformMode& value)
            : EnumAttributeAdapterBase<op::v4::Interpolate::CoordinateTransformMode>(value)
        {
        }

        static constexpr DiscreteTypeInfo type_info{
            "AttributeAdapter<op::v4::Interpolate::CoordinateTransformMode>", 4};
        const DiscreteTypeInfo& get_type_info() const override { return type_info; }
    };

    NGRAPH_API
    std::ostream& operator<<(std::ostream& s, const op::v4::Interpolate::NearestMode& type);

    template <>
    class NGRAPH_API AttributeAdapter<op::v4::Interpolate::NearestMode>
        : public EnumAttributeAdapterBase<op::v4::Interpolate::NearestMode>
    {
    public:
        AttributeAdapter(op::v4::Interpolate::NearestMode& value)
            : EnumAttributeAdapterBase<op::v4::Interpolate::NearestMode>(value)
        {
        }

        static constexpr DiscreteTypeInfo type_info{
            "AttributeAdapter<op::v4::Interpolate::NearestMode>", 4};
        const DiscreteTypeInfo& get_type_info() const override { return type_info; }
    };

    template <>
    class NGRAPH_API AttributeAdapter<op::v4::Interpolate::InterpolateAttrs> : public VisitorAdapter
    {
    public:
        AttributeAdapter(op::v4::Interpolate::InterpolateAttrs& ref);

        virtual bool visit_attributes(AttributeVisitor& visitor) override;
        static constexpr DiscreteTypeInfo type_info{
            "AttributeAdapter<op::v4::Interpolate::InterpolateAttrs>", 4};
        const DiscreteTypeInfo& get_type_info() const override { return type_info; }
    protected:
        op::v4::Interpolate::InterpolateAttrs& m_ref;
    };
} // namespace ngraph
