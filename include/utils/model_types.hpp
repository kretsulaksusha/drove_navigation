#ifndef DRONE_NAVIGATION_MODEL_TYPES_HPP
#define DRONE_NAVIGATION_MODEL_TYPES_HPP

enum class ModelType {
    DAV2,     // Depth Anything v2
    DAD,      // Distill Any Depth
    MiDaS,    // MiDaS
    Unknown,
};

/**
 * Convert a ModelType enum value to its string representation.
 *
 * @param model_type The ModelType enum value (e.g., DAV2, DAD, MiDaS).
 * @return The string representation of the model type (e.g., "dav2", "dad", "midas").
 */
inline std::string modelTypeToString(ModelType model_type) {
    switch (model_type) {
        case ModelType::DAV2: return "dav2";
        case ModelType::DAD: return "dad";
        case ModelType::MiDaS: return "midas";
        default: return "unknown";
    }
}

/**
 * Convert a string to a ModelType enum value.
 *
 * @param model_type_str The string representation of the model type (e.g., "dav2", "dad", "midas").
 * @return The corresponding ModelType enum value.
 */
inline ModelType stringToModelType(const std::string& model_type_str) {
    if (model_type_str == "dav2") return ModelType::DAV2;
    if (model_type_str == "dad") return ModelType::DAD;
    if (model_type_str == "midas") return ModelType::MiDaS;
    return ModelType::Unknown;
}


#endif //DRONE_NAVIGATION_MODEL_TYPES_HPP
