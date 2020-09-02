// Copyright (C) 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include <string>
#include "ngraph_reader_tests.hpp"
TEST_F(NGraphReaderTests, ReadInterpolateNetwork) {
    std::string model = R"V0G0N(
<net name="Network" version="10">
    <layers>
        <layer id="0" name="data" type="Parameter" precision="FP32" version="opset1">
            <data element_type="f32" shape="1,2,48,80"/>
            <output>
                <port id="0" precision="FP32">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>48</dim>
                    <dim>80</dim>
                </port>
            </output>
        </layer>
        <layer id="1" name="const1" type="Const" precision="I64" version="opset1">
            <data offset="0" size="16"/>
            <output>
                <port id="1" precision="I64">
                    <dim>2</dim>
                </port>
            </output>
        </layer>
        <layer id="3" name="interpolate" type="Interpolate" precision="FP32" version="opset1">
            <data axes="2,3" align_corners="0" pads_begin="0" pads_end="0" mode="linear"/>
            <input>
                <port id="0">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>48</dim>
                    <dim>80</dim>
                </port>
                <port id="1">
                    <dim>2</dim>
                </port>
            </input>
            <output>
                <port id="3" precision="FP32">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>50</dim>
                    <dim>60</dim>
                </port>
            </output>
        </layer>
        <layer name="output" type="Result" id="2" precision="FP32" version="opset1">
            <input>
                <port id="0">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>50</dim>
                    <dim>60</dim>
                </port>
            </input>
        </layer>
    </layers>
    <edges>
        <edge from-layer="0" from-port="0" to-layer="3" to-port="0"/>
        <edge from-layer="1" from-port="1" to-layer="3" to-port="1"/>
        <edge from-layer="3" from-port="3" to-layer="2" to-port="0"/>
    </edges>
</net>
)V0G0N";
    std::string modelV5 = R"V0G0N(
<net name="Network" version="5" precision="FP32" batch="1">
    <layers>
        <layer id="0" name="data" precision="FP32" type="Input">
            <output>
                <port id="0">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>48</dim>
                    <dim>80</dim>
                </port>
            </output>
        </layer>
        <layer id="1" name="interpolate" precision="FP32" type="Interp">
            <data align_corners="0" pad_beg="0" pad_end="0" mode="linear" width="60" height="50"/>
            <input>
                <port id="0">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>48</dim>
                    <dim>80</dim>
                </port>
            </input>
            <output>
                <port id="1">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>50</dim>
                    <dim>60</dim>
                </port>
            </output>
        </layer>
    </layers>
    <edges>
        <edge from-layer="0" from-port="0" to-layer="1" to-port="0"/>
    </edges>
</net>
)V0G0N";
    compareIRs(model, modelV5, 16, [](Blob::Ptr& weights) {
                auto *data = weights->buffer().as<int64_t *>();
                data[0] = 50;
                data[1] = 60;
            });
}
TEST_F(NGraphReaderTests, ReadInterpolate2Network) {
    std::string model = R"V0G0N(
<net name="Network" version="10">
    <layers>
        <layer id="0" name="data" type="Parameter" precision="FP32" version="opset1">
            <data element_type="f32" shape="1,2,48,80"/>
            <output>
                <port id="0" precision="FP32">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>48</dim>
                    <dim>80</dim>
                </port>
            </output>
        </layer>
        <layer id="1" name="const1" type="Const" precision="I64" version="opset1">
            <data offset="0" size="32"/>
            <output>
                <port id="1" precision="I64">
                    <dim>4</dim>
                </port>
            </output>
        </layer>
        <layer id="3" name="interpolate" type="Interpolate" precision="FP32" version="opset1">
            <data axes="0,2,1,3" align_corners="0" pads_begin="0" pads_end="0" mode="linear"/>
            <input>
                <port id="0">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>48</dim>
                    <dim>80</dim>
                </port>
                <port id="1">
                    <dim>4</dim>
                </port>
            </input>
            <output>
                <port id="3" precision="FP32">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>50</dim>
                    <dim>60</dim>
                </port>
            </output>
        </layer>
        <layer name="output" type="Result" id="2" precision="FP32" version="opset1">
            <input>
                <port id="0">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>50</dim>
                    <dim>60</dim>
                </port>
            </input>
        </layer>
    </layers>
    <edges>
        <edge from-layer="0" from-port="0" to-layer="3" to-port="0"/>
        <edge from-layer="1" from-port="1" to-layer="3" to-port="1"/>
        <edge from-layer="3" from-port="3" to-layer="2" to-port="0"/>
    </edges>
</net>
)V0G0N";
    std::string modelV5 = R"V0G0N(
<net name="Network" version="5" precision="FP32" batch="1">
    <layers>
        <layer id="0" name="data" precision="FP32" type="Input">
            <output>
                <port id="0">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>48</dim>
                    <dim>80</dim>
                </port>
            </output>
        </layer>
        <layer id="1" name="interpolate" precision="FP32" type="Interp">
            <data align_corners="0" pad_beg="0" pad_end="0" mode="linear" width="60" height="50"/>
            <input>
                <port id="0">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>48</dim>
                    <dim>80</dim>
                </port>
            </input>
            <output>
                <port id="1">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>50</dim>
                    <dim>60</dim>
                </port>
            </output>
        </layer>
    </layers>
    <edges>
        <edge from-layer="0" from-port="0" to-layer="1" to-port="0"/>
    </edges>
</net>
)V0G0N";
    compareIRs(model, modelV5, 32, [](Blob::Ptr& weights) {
                auto *data = weights->buffer().as<int64_t *>();
                data[0] = 1;
                data[1] = 2;
                data[2] = 50;
                data[3] = 60;
            });
}
TEST_F(NGraphReaderTests, ReadInterpolate4Network) {
    std::string model = R"V0G0N(
<net name="Network" version="10">
    <layers>
        <layer id="0" name="data" type="Parameter" precision="FP32" version="opset1">
            <data element_type="f32" shape="1,2,48,80"/>
            <output>
                <port id="0" precision="FP32">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>48</dim>
                    <dim>80</dim>
                </port>
            </output>
        </layer>
        <layer id="1" name="sizes" type="Const" version="opset1">
            <data element_type="i64" offset="0" shape="2" size="16"/>
            <output>
                <port id="1" precision="I64">
                    <dim>2</dim>
                </port>
            </output>
        </layer>
        <layer id="2" name="scales" type="Const" version="opset1">
            <data element_type="f32" offset="16" shape="2" size="8"/>
            <output>
                <port id="1" precision="FP32">
                    <dim>2</dim>
                </port>
            </output>
        </layer>
        <layer id="3" name="axes" type="Const" version="opset1">
            <data element_type="i64" offset="24" shape="2" size="16"/>
            <output>
                <port id="1" precision="I64">
                    <dim>2</dim>
                </port>
            </output>
        </layer>
        <layer id="5" name="interpolate" type="Interpolate" version="opset4">
            <data antialias="0" mode="linear" pads_begin="0,0,0,0" pads_end="0,0,0,0" shape_calculation_mode="sizes"/>
            <input>
                <port id="0">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>48</dim>
                    <dim>80</dim>
                </port>
                <port id="1">
                    <dim>2</dim>
                </port>
                <port id="2">
                    <dim>2</dim>
                </port>
                <port id="3">
                    <dim>2</dim>
                </port>
            </input>
            <output>
                <port id="4" precision="FP32">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>50</dim>
                    <dim>60</dim>
                </port>
            </output>
        </layer>
        <layer name="output" type="Result" id="4" precision="FP32" version="opset1">
            <input>
                <port id="0">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>50</dim>
                    <dim>60</dim>
                </port>
            </input>
        </layer>
    </layers>
    <edges>
        <edge from-layer="0" from-port="0" to-layer="5" to-port="0"/>
        <edge from-layer="1" from-port="1" to-layer="5" to-port="1"/>
        <edge from-layer="2" from-port="1" to-layer="5" to-port="2"/>
        <edge from-layer="3" from-port="1" to-layer="5" to-port="3"/>
        <edge from-layer="5" from-port="4" to-layer="4" to-port="0"/>
    </edges>
</net>
)V0G0N";
    std::string modelV5 = R"V0G0N(
<net name="Network" version="5" precision="FP32" batch="1">
    <layers>
        <layer id="0" name="data" precision="FP32" type="Input">
            <output>
                <port id="0">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>48</dim>
                    <dim>80</dim>
                </port>
            </output>
        </layer>
        <layer id="1" name="interpolate" precision="FP32" type="Interp">
            <data align_corners="0" pad_beg="0" pad_end="0" mode="linear" width="60" height="50"/>
            <input>
                <port id="0">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>48</dim>
                    <dim>80</dim>
                </port>
            </input>
            <output>
                <port id="1">
                    <dim>1</dim>
                    <dim>2</dim>
                    <dim>50</dim>
                    <dim>60</dim>
                </port>
            </output>
        </layer>
    </layers>
    <edges>
        <edge from-layer="0" from-port="0" to-layer="1" to-port="0"/>
    </edges>
</net>
)V0G0N";
    compareIRs(model, modelV5, 40, [](Blob::Ptr& weights) {
                auto *sizes = weights->buffer().as<int64_t *>();
                sizes[0] = 50;
                sizes[1] = 60;
                auto *scales = reinterpret_cast<float *>(sizes + 2);
                scales[0] = 50.0f / 48.0f;
                scales[1] = 60.0f / 80.0f;
                auto *axes = reinterpret_cast<int64_t *>(scales + 2);
                axes[0] = 2;
                axes[1] = 3;
            });
}
