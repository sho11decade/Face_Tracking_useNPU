#include <cassert>
#include <vector>

#include "npu_vtube/inference/inference_service.hpp"
#include "npu_vtube/mapping/parameter_mapper.hpp"
#include "npu_vtube/tracking/tracking_service.hpp"

int main() {
    npuvt::inference::InferenceService inference_service;
    npuvt::tracking::TrackingService tracking_service;
    npuvt::mapping::ParameterMapper parameter_mapper;

    const auto device = inference_service.select_device(std::vector<std::string> {"CPU", "GPU"});
    assert(device == "GPU");

    const auto observation = inference_service.make_placeholder_observation();
    const auto tracking_state = tracking_service.update(observation);
    const auto mapped = parameter_mapper.map(tracking_state);

    assert(tracking_state.valid);
    assert(mapped.head_z > 0.0F);
    return 0;
}
