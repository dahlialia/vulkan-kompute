/* summator.cpp */

#include <vector>
#include <string>
#include <iostream>

#include "KomputeSummator.hpp"

namespace godot {

KomputeSummator::KomputeSummator() {

}

void KomputeSummator::train(Array yArr, Array xIArr, Array xJArr) {

    assert(y.size() == xI.size());
    assert(xI.size() == xJ.size());

    std::vector<float> yData;
    std::vector<float> xIData;
    std::vector<float> xJData;
    std::vector<float> zerosData;

    for (int i = 0; i < yArr.size(); i++) {
        yData.push_back(yArr[i]);
        xIData.push_back(xIArr[i]);
        xJData.push_back(xJArr[i]);
        zerosData.push_back(0);
    }

    uint32_t ITERATIONS = 100;
    float learningRate = 0.1;

    std::shared_ptr<kp::Tensor> xI{ new kp::Tensor(xIData) };
    std::shared_ptr<kp::Tensor> xJ{ new kp::Tensor(xJData) };

    std::shared_ptr<kp::Tensor> y{ new kp::Tensor(yData) };

    std::shared_ptr<kp::Tensor> wIn{ new kp::Tensor({ 0.001, 0.001 }) };
    std::shared_ptr<kp::Tensor> wOutI{ new kp::Tensor(zerosData) };
    std::shared_ptr<kp::Tensor> wOutJ{ new kp::Tensor(zerosData) };

    std::shared_ptr<kp::Tensor> bIn{ new kp::Tensor({ 0 }) };
    std::shared_ptr<kp::Tensor> bOut{ new kp::Tensor(zerosData) };

    std::shared_ptr<kp::Tensor> lOut{ new kp::Tensor(zerosData) };

    std::vector<std::shared_ptr<kp::Tensor>> params = { xI,  xJ,    y,
                                                        wIn, wOutI, wOutJ,
                                                        bIn, bOut,  lOut };

    {
        kp::Manager mgr;

        if (std::shared_ptr<kp::Sequence> sq =
              mgr.getOrCreateManagedSequence("createTensors").lock()) {

            sq->begin();

            sq->record<kp::OpTensorCreate>(params);

            sq->end();
            sq->eval();

            // Record op algo base
            sq->begin();

            sq->record<kp::OpTensorSyncDevice>({ wIn, bIn });

            sq->record<kp::OpAlgoBase<>>(
              params, std::vector<char>(LR_SHADER.begin(), LR_SHADER.end()));

            sq->record<kp::OpTensorSyncLocal>({ wOutI, wOutJ, bOut, lOut });

            sq->end();

            // Iterate across all expected iterations
            for (int i = 0; i < ITERATIONS; i++) {

                sq->eval();

                for (int j = 0; j < bOut->size(); j++) {
                    wIn->data()[0] -= learningRate * wOutI->data()[j];
                    wIn->data()[1] -= learningRate * wOutJ->data()[j];
                    bIn->data()[0] -= learningRate * bOut->data()[j];
                }
            }
        }
    }

    SPDLOG_INFO("RESULT: <<<<<<<<<<<<<<<<<<<");
    SPDLOG_INFO(wIn->data()[0]);
    SPDLOG_INFO(wIn->data()[1]);
    SPDLOG_INFO(bIn->data()[0]);

    this->mWeights = kp::Tensor(wIn->data());
    this->mBias = kp::Tensor(bIn->data());
}

Array KomputeSummator::predict(Array xI, Array xJ) {
    assert(xI.size() == xJ.size());

    Array retArray;

    // We run the inference in the CPU for simplicity
    // BUt you can also implement the inference on GPU 
    // GPU implementation would speed up minibatching
    for (int i = 0; i < xI.size(); i++) {
        float xIVal = xI[i];
        float xJVal = xJ[i];
        float result = (xIVal * this->mWeights.data()[0]
                + xJVal * this->mWeights.data()[1]
                + this->mBias.data()[0]);

        // Instead of using sigmoid we'll just return full numbers
        Variant var = result > 0 ? 1 : 0;
        retArray.push_back(var);
    }

    return retArray;
}

void KomputeSummator::_register_methods() {
    register_method((char *)"train", &KomputeSummator::train);
    register_method((char *)"predict", &KomputeSummator::predict);
}

}

