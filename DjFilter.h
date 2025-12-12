#pragma once

#include "Commons.h"
#include "StateVariableFilter.h"

class DjFilter
{
private:
    enum FilterType
    {
        NO_FILTER,
        LP,
        HP,
    };

    StateVariableFilter* lpfs_[2];
    StateVariableFilter* hpfs_[2];

    FilterType filter_ = FilterType::NO_FILTER;
    
    float freq_;
    float reso_;
    float lpfMix_;
    float hpfMix_;
    float amp_;

    void UpdateFilter()
    {
        switch (filter_)
        {
        case FilterType::LP:
            lpfs_[LEFT_CHANNEL]->setLowPass(freq_, reso_);
            lpfs_[RIGHT_CHANNEL]->setLowPass(freq_, reso_);
            break;
        case FilterType::HP:
            hpfs_[LEFT_CHANNEL]->setHighPass(freq_, reso_);
            hpfs_[RIGHT_CHANNEL]->setHighPass(freq_, reso_);
            break;

        default:
            break;
        }
    }

public:
    DjFilter(float sampleRate)
    {
        for (size_t i = 0; i < 2; i++)
        {
            lpfs_[i] = StateVariableFilter::create(sampleRate);
            hpfs_[i] = StateVariableFilter::create(sampleRate);
        }

        filter_ = FilterType::NO_FILTER;
        reso_ = 0.f;
        lpfMix_ = 0.f;
        hpfMix_ = 0.f;
        amp_ = 1.f;
    }
    ~DjFilter()
    {
        for (size_t i = 0; i < 2; i++)
        {
            StateVariableFilter::destroy(lpfs_[i]);
            StateVariableFilter::destroy(hpfs_[i]);
        }
    }

    static DjFilter* create(float sampleRate)
    {
        return new DjFilter(sampleRate);
    }

    static void destroy(DjFilter* obj)
    {
        delete obj;
    }

    void SetFilter(float value)
    {
        if (value <= 0.45f)
        {
            filter_ = FilterType::LP;
            lpfMix_ = Map(value, 0.f, 0.45f, 1.f, 0.f);
            freq_ = Map(lpfMix_, 0.f, 1.f, 200.f, 30.f);
            reso_ = Map(lpfMix_, 0.f, 1.f, 0.1f, FilterStage::BUTTERWORTH_Q);
            UpdateFilter();
            amp_ = Map(lpfMix_, 0.f, 1.f, kDjFilterMakeupGainMin, kDjFilterMakeupGainMaxLp);
        }
        else if (value >= 0.55f)
        {
            filter_ = FilterType::HP;
            hpfMix_ = Map(value, 0.55f, 1.f, 0.f, 1.f);
            freq_ = Map(hpfMix_, 0.f, 1.f, 2000.f, 4000.f);
            reso_ = Map(hpfMix_, 0.f, 1.f, 0.1f, FilterStage::BUTTERWORTH_Q);
            UpdateFilter();
            amp_ = Map(hpfMix_, 0.f, 1.f, kDjFilterMakeupGainMin, kDjFilterMakeupGainMaxHp);
        }
        else
        {
            filter_ = FilterType::NO_FILTER;
        }
    }

    void Process(float leftIn, float rightIn, float &leftOut, float &rightOut)
    {
        switch (filter_)
        {
        case FilterType::LP:
            leftOut = LinearCrossFade(leftIn, lpfs_[LEFT_CHANNEL]->process(leftIn), lpfMix_);
            rightOut = LinearCrossFade(rightIn, lpfs_[RIGHT_CHANNEL]->process(rightIn), lpfMix_);
            break;
        case FilterType::HP:
            leftOut = LinearCrossFade(leftIn, hpfs_[LEFT_CHANNEL]->process(leftIn), hpfMix_);
            rightOut = LinearCrossFade(rightIn, hpfs_[RIGHT_CHANNEL]->process(rightIn), hpfMix_);
            break;
        default:
            leftOut = leftIn;
            rightOut = rightIn;
            break;
        }

        leftOut = Clamp(leftOut, -3.f, 3.f);
        rightOut = Clamp(rightOut, -3.f, 3.f);
    }

    void Process(AudioBuffer &input, AudioBuffer &output)
    {
        size_t size = output.getSize();

        FloatArray leftIn = input.getSamples(LEFT_CHANNEL);
        FloatArray rightIn = input.getSamples(RIGHT_CHANNEL);
        FloatArray leftOut = output.getSamples(LEFT_CHANNEL);
        FloatArray rightOut = output.getSamples(RIGHT_CHANNEL);

        for (size_t i = 0; i < size; i++)
        {
            Process(leftIn[i] * amp_, rightIn[i] * amp_, leftOut[i], rightOut[i]);
        }
    }
};