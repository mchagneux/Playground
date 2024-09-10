#include "./NeuralParameters.h"

juce::AudioProcessorValueTreeState::ParameterLayout NeuralParameters::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;


    params.push_back (std::make_unique<juce::AudioParameterChoice> (BACKEND_TYPE_ID,
                                                                    BACKEND_TYPE_NAME,
                                                                    backendTypes,
                                                                    backendTypes.indexOf(defaultBackend)));

    params.push_back( std::make_unique<juce::AudioParameterFloat>  (DRY_WET_ID,
                                                                    DRY_WET_NAME,
                                                                    dryWetRange,
                                                                    1.0f));


    if (parameterList.isEmpty()) {
        for (const auto & param : params) {
            parameterList.add(param->getParameterID());
        }
    }

    return { params.begin(), params.end() };
}

juce::StringArray NeuralParameters::getPluginParameterList() {
    return parameterList;
}
