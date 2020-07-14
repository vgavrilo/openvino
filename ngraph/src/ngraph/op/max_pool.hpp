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

#include "ngraph/op/op.hpp"
#include "ngraph/op/util/attr_types.hpp"

namespace ngraph
{
    namespace op
    {
        namespace v1
        {
            /// \brief Batched max pooling operation.
            class NGRAPH_API MaxPool : public Op
            {
            public:
                static constexpr NodeTypeInfo type_info{"MaxPool", 1};
                const NodeTypeInfo& get_type_info() const override { return type_info; }
                /// \brief Constructs a batched max pooling operation.
                MaxPool() = default;

                /// \brief Constructs a batched max pooling operation.
                ///
                /// \param arg The node producing the input data batch tensor.
                /// \param strides The strides.
                /// \param pads_begin The beginning of padding shape.
                /// \param pads_end The end of padding shape.
                /// \param kernel The kernel shape.
                /// \param rounding_mode Whether to use ceiling or floor rounding type while
                /// computing output shape.
                /// \param auto_pad The pad type for automatically computing padding sizes.
                MaxPool(const Output<Node>& arg,
                        const Strides& strides,
                        const Shape& pads_begin,
                        const Shape& pads_end,
                        const Shape& kernel,
                        op::RoundingType rounding_mode,
                        const PadType& auto_pad);

                /// \brief Constructs a batched max pooling operation.
                ///
                /// \param arg The node producing the input data batch tensor.
                /// \param strides The strides.
                /// \param pads_begin The beginning of padding shape.
                /// \param pads_end The end of padding shape.
                /// \param kernel The kernel shape.
                /// \param rounding_mode Whether to use ceiling or floor rounding type while
                /// computing output shape.
                MaxPool(const Output<Node>& arg,
                        const Strides& strides,
                        const Shape& pads_begin,
                        const Shape& pads_end,
                        const Shape& kernel,
                        op::RoundingType rounding_mode);

                bool visit_attributes(AttributeVisitor& visitor) override;
                size_t get_version() const override { return 1; }
                void validate_and_infer_types() override;

                virtual std::shared_ptr<Node>
                    clone_with_new_inputs(const OutputVector& new_args) const override;

                /// \return The kernel shape.
                const Shape& get_kernel() const { return m_kernel; }
                void set_kernel(const Shape& kernel) { m_kernel = kernel; }
                /// \return The strides.
                const Strides& get_strides() const { return m_strides; }
                void set_strides(const Strides& strides) { m_strides = strides; }
                /// \return The beginning of padding shape.
                const Shape& get_pads_begin() const { return m_pads_begin; }
                void set_pads_begin(const Shape& pads_begin) { m_pads_begin = pads_begin; }
                /// \return The end of padding shape.
                const Shape& get_pads_end() const { return m_pads_end; }
                void set_adding_above(const Shape& pads_end) { m_pads_end = pads_end; }
                /// \return The pad type for pooling.
                const PadType& get_auto_pad() const { return m_auto_pad; }
                void set_auto_pad(const PadType& auto_pad) { m_auto_pad = auto_pad; }
                /// \return The ceiling mode being used for output shape computations
                op::RoundingType get_rounding_type() const { return m_rounding_type; }
                void set_rounding_type(op::RoundingType rounding_mode)
                {
                    m_rounding_type = rounding_mode;
                }
                /// \return The default value for MaxPool.
                virtual std::shared_ptr<Node> get_default_value() const override;

                bool evaluate(const HostTensorVector& outputs,
                              const HostTensorVector& inputs) override;

            protected:
                Shape m_kernel;
                Strides m_strides;
                Shape m_pads_begin;
                Shape m_pads_end;
                PadType m_auto_pad;
                op::RoundingType m_rounding_type{op::RoundingType::FLOOR};

            private:
                bool update_auto_padding(const PartialShape& in_shape,
                                         Shape& new_pads_end,
                                         Shape& new_pads_begin);
            };
        } // namespace v1
    }     // namespace op
} // namespace ngraph
