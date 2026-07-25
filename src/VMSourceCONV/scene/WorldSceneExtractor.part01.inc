            return false;
        }
        output.push_back(number);
    }
    return true;
}

[[nodiscard]] double Length3(
    const double x,
    const double y,
    const double z) {
    return std::sqrt(x * x + y * y + z * z);
}

[[nodiscard]] Quaternion QuaternionFromRotation(
    const std::array<double, 16>& matrix) {
    Quaternion result;
    const auto trace = matrix[0] + matrix[5] + matrix[10];
    if (trace > 0.0) {
        const auto s = std::sqrt(trace + 1.0) * 2.0;
        result.w = 0.25 * s;
        result.x = (matrix[9] - matrix[6]) / s;
        result.y = (matrix[2] - matrix[8]) / s;
        result.z = (matrix[4] - matrix[1]) / s;
    } else if (matrix[0] > matrix[5] && matrix[0] > matrix[10]) {
        const auto s = std::sqrt(1.0 + matrix[0] - matrix[5] - matrix[10]) * 2.0;
        result.w = (matrix[9] - matrix[6]) / s;
        result.x = 0.25 * s;
        result.y = (matrix[1] + matrix[4]) / s;
        result.z = (matrix[2] + matrix[8]) / s;
    } else if (matrix[5] > matrix[10]) {
        const auto s = std::sqrt(1.0 + matrix[5] - matrix[0] - matrix[10]) * 2.0;
        result.w = (matrix[2] - matrix[8]) / s;
        result.x = (matrix[1] + matrix[4]) / s;
        result.y = 0.25 * s;
        result.z = (matrix[6] + matrix[9]) / s;
    } else {
        const auto s = std::sqrt(1.0 + matrix[10] - matrix[0] - matrix[5]) * 2.0;
        result.w = (matrix[4] - matrix[1]) / s;
        result.x = (matrix[2] + matrix[8]) / s;
        result.y = (matrix[6] + matrix[9]) / s;
        result.z = 0.25 * s;
    }
    return result;
}

void DecomposeTransform(Transform& transform) {
    transform.origin = {
        transform.matrix[12],
        transform.matrix[13],
        transform.matrix[14],
    };

    transform.scale = {
        Length3(transform.matrix[0], transform.matrix[1], transform.matrix[2]),
        Length3(transform.matrix[4], transform.matrix[5], transform.matrix[6]),
        Length3(transform.matrix[8], transform.matrix[9], transform.matrix[10]),
    };

    constexpr double Epsilon = 1.0e-12;
    if (transform.scale.x <= Epsilon
        || transform.scale.y <= Epsilon
        || transform.scale.z <= Epsilon) {
        return;
    }

    auto rotation = transform.matrix;
    rotation[0] /= transform.scale.x;
    rotation[1] /= transform.scale.x;
    rotation[2] /= transform.scale.x;
    rotation[4] /= transform.scale.y;
    rotation[5] /= transform.scale.y;
    rotation[6] /= transform.scale.y;
    rotation[8] /= transform.scale.z;
    rotation[9] /= transform.scale.z;
    rotation[10] /= transform.scale.z;

    const auto determinant =
        rotation[0] * (rotation[5] * rotation[10] - rotation[6] * rotation[9])
        - rotation[1] * (rotation[4] * rotation[10] - rotation[6] * rotation[8])
        + rotation[2] * (rotation[4] * rotation[9] - rotation[5] * rotation[8]);
    if (determinant < 0.0) {
        transform.scale.z = -transform.scale.z;
        rotation[8] = -rotation[8];
        rotation[9] = -rotation[9];
        rotation[10] = -rotation[10];
    }

    transform.rotation = QuaternionFromRotation(rotation);
    transform.decompositionSucceeded = true;
}

[[nodiscard]] bool TryTransform(const Kv3Value& value, Transform& output) {
    if (value.Type() != Kv3ValueType::Array) {
        return false;
    }

    std::array<double, 16> matrix{
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    };

    std::vector<double> flat;
    if (TryNumericArray(value, flat) && flat.size() >= 12U) {
        matrix[0] = flat[0];
        matrix[1] = flat[4];
        matrix[2] = flat[8];
        matrix[4] = flat[1];
        matrix[5] = flat[5];
        matrix[6] = flat[9];
        matrix[8] = flat[2];
        matrix[9] = flat[6];
        matrix[10] = flat[10];
        matrix[12] = flat[3];
        matrix[13] = flat[7];
        matrix[14] = flat[11];
        if (flat.size() >= 16U) {
            matrix[3] = flat[12];
            matrix[7] = flat[13];
            matrix[11] = flat[14];
            matrix[15] = flat[15];
        }
    } else {
        const auto& columns = value.ArrayValues();
        if (columns.size() < 3U) {
            return false;
        }
        std::array<Vector4, 4> vectors{};
        vectors[3].w = 1.0;
        if (!TryVector4(columns[0], vectors[0])
            || !TryVector4(columns[1], vectors[1])
            || !TryVector4(columns[2], vectors[2])) {
            return false;
        }
        if (columns.size() >= 4U && !TryVector4(columns[3], vectors[3])) {
            return false;
        }
        matrix = {
            vectors[0].x, vectors[1].x, vectors[2].x, vectors[3].x,
            vectors[0].y, vectors[1].y, vectors[2].y, vectors[3].y,
            vectors[0].z, vectors[1].z, vectors[2].z, vectors[3].z,
            vectors[0].w, vectors[1].w, vectors[2].w, vectors[3].w,
        };
    }

    output.present = true;
    output.matrix = matrix;
    DecomposeTransform(output);
    return true;
}

[[nodiscard]] bool TryBounds(const Kv3Value& value, Bounds& output) {
    if (value.Type() == Kv3ValueType::Array
        && value.ArrayValues().size() >= 2U
        && TryVector3(value.ArrayValues()[0], output.minimum)
        && TryVector3(value.ArrayValues()[1], output.maximum)) {
        output.present = true;
        return true;
    }
    if (value.Type() == Kv3ValueType::Object) {
        const auto* minimum = value.Find("m_vMinBounds");
        const auto* maximum = value.Find("m_vMaxBounds");
        if (minimum != nullptr && maximum != nullptr
            && TryVector3(*minimum, output.minimum)
            && TryVector3(*maximum, output.maximum)) {
            output.present = true;
            return true;
        }
