#ifndef __Iroi_1_0_0Patch_hpp__
#define __Iroi_1_0_0Patch_hpp__

#include "Commons.h"
#include "Ui.h"
#include "Clock.h"
#include "InputDetector.h"

class Iroi_1_0_0Patch : public Patch {
private:
    Ui* ui_;
    Iroi* iroi_;
    Clock* clock_;
    InputDetector* inDetec_;

    PatchCtrls patchCtrls;
    PatchCvs patchCvs;
    PatchState patchState;

public:
    Iroi_1_0_0Patch()
    {
        patchState.sampleRate = getSampleRate();
        patchState.blockRate = getBlockRate();
        patchState.blockSize = getBlockSize();
        ui_ = Ui::create(&patchCtrls, &patchCvs, &patchState);
        iroi_ = Iroi::create(&patchCtrls, &patchCvs, &patchState);
        clock_ = Clock::create(&patchCtrls, &patchState);
        inDetec_ = InputDetector::create(&patchCtrls, &patchState);
    }
    ~Iroi_1_0_0Patch()
    {
        Iroi::destroy(iroi_);
        Ui::destroy(ui_);
        Clock::destroy(clock_);
        InputDetector::destroy(inDetec_);
    }

    void buttonChanged(PatchButtonId bid, uint16_t value, uint16_t samples) override
    {
        ui_->ProcessButton(bid, value, samples);
    }

    void processMidi(MidiMessage msg) override
    {
        ui_->ProcessMidi(msg);
    }

    void processAudio(AudioBuffer& buffer) override
    {
        ui_->Poll();
        clock_->Process();
        //inDetec_->Process(buffer);
        iroi_->Process(buffer);
    }
};

#endif // __Iroi_1_0_0Patch_hpp__
