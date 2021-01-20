## ExperimentalDetectronDetectionOutput <a name="ExperimentalDetectronDetectionOutput"></a> {#openvino_docs_ops_detection_ExperimentalDetectronDetectionOutput_6}

**Versioned name**: *ExperimentalDetectronDetectionOutput-6*

**Category**: *Object detection*

**Short description**: *ExperimentalDetectronDetectionOutput* performs non-maximum suppression to generate the detection output using information on location and confidence predictions.

**Detailed description**: [Reference](https://arxiv.org/pdf/1512.02325.pdf). The layer has 3 mandatory inputs: tensor with box logits, tensor with confidence predictions and tensor with box coordinates (proposals). It can have 2 additional inputs with additional confidence predictions and box coordinates described in the [article](https://arxiv.org/pdf/1711.06897.pdf). The output tensor contains information about filtered detections described with 7 element tuples: `[batch_id, class_id, confidence, x_1, y_1, x_2, y_2]`. The first tuple with `batch_id` equal to `-1` means end of output.

At each feature map cell, *ExperimentalDetectronDetectionOutput* predicts the offsets relative to the default box shapes in the cell, as well as the per-class scores that indicate the presence of a class instance in each of those boxes. Specifically, for each box out of k at a given location, *DetectionOutput* computes class scores and the four offsets relative to the original default box shape. This results in a total of \f$(c + 4)k\f$ filters that are applied around each location in the feature map, yielding \f$(c + 4)kmn\f$ outputs for a *m \* n* feature map.

**Attributes**:
