#pragma once

#include "Commons.h"
#include "SquareWaveOscillator.h"
#include <array>

extern PatchProcessor* getInitialisingPatchProcessor();

class InputDetector
{
private:
    PatchCtrls* patchCtrls_;
    PatchState* patchState_;

    SquareWaveOscillator* osc_;

    std::array<bool, 32> sequence = {
        1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1,
        0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0
    }; 

    std::array<bool, 5> read;

    int p = 0;

    float score_;
    float lp_coefficient_;
    float threshold_;

    bool state_;

      int normalization_detection_count_;
  int normalization_detection_mismatches_;
  uint32_t normalization_probe_state_;


const int kProbeSequenceDuration = 32;

public:
    InputDetector(PatchCtrls* patchCtrls, PatchState* patchState)
    {
        patchCtrls_ = patchCtrls;
        patchState_ = patchState;

        osc_ = SquareWaveOscillator::create(patchState_->sampleRate);

        score_ = 0.0f;
        state_ = false;
        threshold_ = 0;
        lp_coefficient_ = 0;

        normalization_detection_count_ = 0;
        normalization_probe_state_ = 0;
        normalization_detection_mismatches_ = 0;
    }
    ~InputDetector() 
    {
        SquareWaveOscillator::destroy(osc_);
    }

    static InputDetector* create(PatchCtrls* patchCtrls, PatchState* patchState)
    {
        return new InputDetector(patchCtrls, patchState);
    }

    static void destroy(InputDetector* obj)
    {
        delete obj;
    }

    /*
    int findPattern(InfiniteStream* stream, vector<int>& pattern) 
    {
        // Initialize hash values for the two halves of the pattern
        long long firstHalfHash = 0;
        long long secondHalfHash = 0;
      
        int patternLength = pattern.size();
        int halfLength = patternLength >> 1;  // Divide by 2 using bit shift
      
        // Create bit masks for the two halves
        // These masks ensure we only keep the relevant bits when updating hashes
        long long firstHalfMask = (1LL << halfLength) - 1;
        long long secondHalfMask = (1LL << (patternLength - halfLength)) - 1;
      
        // Build the hash for the first half of the pattern
        // Each bit is shifted to its position in the hash
        for (int i = 0; i < halfLength; ++i) {
            firstHalfHash |= (long long)pattern[i] << (halfLength - 1 - i);
        }
      
        // Build the hash for the second half of the pattern
        for (int i = halfLength; i < patternLength; ++i) {
            secondHalfHash |= (long long)pattern[i] << (patternLength - 1 - i);
        }
      
        // Rolling hash values for the current window in the stream
        long long currentFirstHalf = 0;
        long long currentSecondHalf = 0;
      
        // Process the stream bit by bit
        for (int position = 1; ; ++position) {
            // Get the next bit from the stream
            int currentBit = stream->next();
          
            // Update the second half hash by shifting left and adding the new bit
            currentSecondHalf = (currentSecondHalf << 1) | currentBit;
          
            // Extract the bit that will overflow from second half to first half
            int overflowBit = (int)((currentSecondHalf >> (patternLength - halfLength)) & 1);
          
            // Keep only the relevant bits in the second half
            currentSecondHalf &= secondHalfMask;
          
            // Update the first half hash with the overflow bit
            currentFirstHalf = (currentFirstHalf << 1) | overflowBit;
            currentFirstHalf &= firstHalfMask;
          
            // Check if we have read enough bits and if the pattern matches
            if (position >= patternLength && 
                firstHalfHash == currentFirstHalf && 
                secondHalfHash == currentSecondHalf) {
                // Return the starting index of the pattern (0-indexed)
                return position - patternLength;
            }
        }
    }
    */

    void Process(float x, float y) 
    {
        // x is supposed to be centered!
        score_ += lp_coefficient_ * (x * y - score_);
        
        float hysteresis = state_ ? -0.05f * threshold_ : 0.0f;
        state_ = score_ >= (threshold_ + hysteresis);
    }

    void Decode(FloatArray buffer)
    {
 
    }

    void Encode(size_t size)
    {
        for (size_t i = 0; i < 32; i++)
        {
            for (size_t j = 0; j < size; j++)
            {
                float freq = sequence[i] * 2000 + 2000;

                osc_->setFrequency(freq);
                bool out = osc_->getSample() > 0;

                getInitialisingPatchProcessor()->patch->setButton(IN_DETEC, out, 0);
            }
        }
    }

    inline void Process(AudioBuffer &buffer)
    {
        FloatArray left = buffer.getSamples(LEFT_CHANNEL);

        Decode(left);

        size_t size = buffer.getSize();

        /*
        for (size_t i = 0; i < size; i++)
        {
            bool expected_value = normalization_probe_state_ >> (size - 1);
            bool read_value = left[i] > 0.25f;
            if (expected_value != read_value) 
            {
                ++normalization_detection_mismatches_;
            }
            ++normalization_detection_count_;
            if (i == size - 1) 
            {
                normalization_detection_count_ = 0;
                bool destination = normalization_detection_mismatches_ >= 2;
                normalization_detection_mismatches_ = 0;
            }
            
            normalization_probe_state_ = 1103515245 * normalization_probe_state_ + 12345;
            getInitialisingPatchProcessor()->patch->setButton(IN_DETEC, normalization_probe_state_ >> (size - 1), 0);
        }
        */

        //Encode(size);
    }
};
