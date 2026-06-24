#include <cassert>
#include <vector>

#include "npu_vtube/capture/capture_service.hpp"
#include "npu_vtube/filter/one_euro_filter.hpp"
#include "npu_vtube/inference/inference_service.hpp"
#include "npu_vtube/mapping/parameter_mapper.hpp"
#include "npu_vtube/models/model_catalog.hpp"
#include "npu_vtube/models/model_resolver.hpp"
#include "npu_vtube/tracking/tracking_service.hpp"

int main() {
    npuvt::filter::OneEuroFilter filter({.min_cutoff = 1.0, .beta = 0.1, .derivative_cutoff = 1.0});
    const auto filtered_a = filter.filter(10.0, 1.0 / 60.0);
    const auto filtered_b = filter.filter(12.0, 1.0 / 60.0);
    assert(filtered_a == 10.0);
    assert(filtered_b > 10.0);

    npuvt::capture::CaptureService capture_service;
    npuvt::inference::InferenceService inference_service;
    npuvt::tracking::TrackingService tracking_service;
    npuvt::mapping::ParameterMapper parameter_mapper;

    npuvt::models::ModelCatalog catalog;
    const auto* face_model = catalog.find(npuvt::models::ModelRole::face_detection);
    assert(face_model != nullptr);
    npuvt::models::ModelResolver resolver({.models_root = "models"});
    const auto resolved_models = resolver.resolve_defaults(catalog);
    assert(!resolved_models.empty());

    const auto device = inference_service.select_device(std::vector<std::string> {"CPU", "GPU"});
    assert(device == "GPU");

    const auto frame = capture_service.make_placeholder_frame();
    const auto observation_from_frame = inference_service.analyze(frame);
    assert(observation_from_frame.frame_width == frame.width);

    const auto resolved_face = inference_service.resolve_model(npuvt::models::ModelRole::face_detection);
    assert(resolved_face.has_value());

    const auto summary = inference_service.load_model_summary();
    assert(summary.face_detection.definition.model_id == "face-detection-0200");

    const auto observation = inference_service.make_placeholder_observation();
    const auto tracking_state = tracking_service.update(observation);
    const auto mapped = parameter_mapper.map(tracking_state);

    assert(tracking_state.valid);
    assert(mapped.head_z > 0.0F);
    assert(mapped.head_x >= -1.0F && mapped.head_x <= 1.0F);
    return 0;
}
