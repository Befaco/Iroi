#pragma once

#include "Iroi.h"
#include "ParamController.h"
#include "Midi.h"
#include "Led.h"
#include "VoltsPerOctave.h"
#include "SquareWaveOscillator.h"
#include "Schmitt.h"
#include "MidiMessage.h"

enum FuncState {
    FUNC_STATE_IDLE,
    FUNC_STATE_PRESSED,
    FUNC_STATE_HELD,
    FUNC_STATE_RELEASED,
};

struct Configuration {
    bool mod_attenuverters;
    bool cv_attenuverters;
    int revision;
};

extern PatchProcessor* getInitialisingPatchProcessor();

class Ui {
private:
    PatchCtrls* patchCtrls_;
    PatchCvs* patchCvs_;
    PatchState* patchState_;

    KnobController* knobs_[PARAM_KNOB_LAST];
    FaderController* faders_[PARAM_FADER_LAST];
    CvController* cvs_[PARAM_CV_LAST];
    SwitchController* switches_[PARAM_SWITCH_LAST];
    MapButtonController* mapButton_;
    RandomButtonController* randomButton_;
    ShiftButtonController* shiftButton_;
    Led* leds_[LED_LAST];
    MidiController* midiOuts_[PARAM_MIDI_LAST];

    CatchUpController* movingParam_;

    Schmitt undoRedoRandomTrigger_, mapAndRandomTrigger_,
        modTypeLockTrigger_, modSpeedLockTrigger_, saveTrigger_;
    Schmitt filterModeTrigger_, filterPositionTrigger_;

    int samplesSinceShiftPressed_, samplesSinceMapOrRandomPressed_,
        samplesSinceMapButtonPressed_, samplesSinceRandomMapInReceived_,
        samplesSinceRandomPressed_;

    int hwRevision_;

    bool mapAndRandomPressed_, fadeOutOutput_,
        fadeInOutput_, parameterChangedSinceLastSave_, saving_, saveFlag_,
        undoRedo_, doRandomSlew_, startup_, mapActive_, mapButtonWasOn_;

    float randomize_, randomSlewInc_, cutoff_, cutoffCv_, cutoffPot_;

    FuncMode selectedMapTarget_;

public:
    Ui(PatchCtrls* patchCtrls, PatchCvs* patchCvs, PatchState* patchState) {
        patchCtrls_ = patchCtrls;
        patchCvs_ = patchCvs;
        patchState_ = patchState;

        movingParam_ = NULL;

        samplesSinceShiftPressed_ = 0;
        samplesSinceMapOrRandomPressed_ = 0;
        samplesSinceMapButtonPressed_ = 0;
        samplesSinceRandomPressed_ = 0;

        mapAndRandomPressed_ = false;
        fadeOutOutput_ = false;
        fadeInOutput_ = false;
        parameterChangedSinceLastSave_ = false;
        saving_ = false;
        saveFlag_ = false;
        randomize_ = false;
        undoRedo_ = false;
        doRandomSlew_ = false;
        startup_ = true;
        mapActive_ = false;
        mapButtonWasOn_ = false;

        selectedMapTarget_ = FuncMode::FUNC_MODE_NONE;

        cutoff_ = 0;
        cutoffCv_ = 0; 
        cutoffPot_ = 0;
        randomSlewInc_ = 0;

        hwRevision_ = 0;

        patchState_->funcMode = FuncMode::FUNC_MODE_NONE;
        patchState_->inputLevel = FloatArray::create(patchState_->blockSize);
        patchState_->outputLevel = FloatArray::create(patchState_->blockSize);
        patchState_->efModLevel = FloatArray::create(patchState_->blockSize);
        patchState_->outLevel = 1.f;
        patchState_->randomSlew = kRandomSlewSamples;
        patchState_->randomHasSlew = false;
        patchState_->modAttenuverters = false;
        patchState_->cvAttenuverters = false;

        for (size_t i = 0; i < PARAM_KNOB_LAST + PARAM_FADER_LAST; i++) {
            patchState_->moving[i] = false;
        }

        // Alt params
        patchCtrls_->filterMode = 0.f;
        patchCtrls_->filterPosition = 0.f;
        patchCtrls_->modType = 0.f;
        patchCtrls_->resonatorDissonance = 0.f;
        patchCtrls_->echoFilter = 0.5f;
        patchCtrls_->ambienceAutoPan = 0.f;

        // Modulation
        patchCtrls_->filterCutoffModAmount = 0.5f;
        patchCtrls_->filterResonanceModAmount = 0.f;
        patchCtrls_->resonatorTuneModAmount = 0.f;
        patchCtrls_->resonatorFeedbackModAmount = 0.f;
        patchCtrls_->echoDensityModAmount = 0.f;
        patchCtrls_->echoRepeatsModAmount = 0.f;
        patchCtrls_->ambienceDecayModAmount = 0.f;
        patchCtrls_->ambienceSpacetimeModAmount = 0.f;

        // CVs
        patchCtrls_->filterCutoffCvAmount = 1.f;
        patchCtrls_->resonatorTuneCvAmount = 1.f;
        patchCtrls_->echoDensityCvAmount = 1.f;
        patchCtrls_->ambienceSpacetimeCvAmount = 1.f;

        // Random map
        patchCtrls_->filterCutoffRndAmount = 0.5f;
        patchCtrls_->filterResonanceRndAmount = 0.5f;
        patchCtrls_->resonatorTuneRndAmount = 0.5f;
        patchCtrls_->resonatorFeedbackRndAmount = 0.5f;
        patchCtrls_->echoDensityRndAmount = 0.5f;
        patchCtrls_->echoRepeatsRndAmount = 0.5f;
        patchCtrls_->ambienceDecayRndAmount = 0.5f;
        patchCtrls_->ambienceSpacetimeRndAmount = 0.5f;

        LoadConfig();

        faders_[PARAM_FADER_FILTER_VOL] =
            FaderController::create(patchState_, &patchCtrls_->filterVol);
        faders_[PARAM_FADER_RESONATOR_VOL] =
            FaderController::create(patchState_, &patchCtrls_->resonatorVol);
        faders_[PARAM_FADER_ECHO_VOL] =
            FaderController::create(patchState_, &patchCtrls_->echoVol);
        faders_[PARAM_FADER_AMBIENCE_VOL] =
            FaderController::create(patchState_, &patchCtrls_->ambienceVol);

        knobs_[PARAM_KNOB_FILTER_CUTOFF] = KnobController::create(patchState_,
            &cutoff_, &patchCtrls_->filterMode,
            &patchCtrls_->filterCutoffModAmount, &patchCtrls_->filterCutoffCvAmount, &patchCtrls_->filterCutoffRndAmount);
        knobs_[PARAM_KNOB_FILTER_RESONANCE] = KnobController::create(patchState_,
            &patchCtrls_->filterResonance, &patchCtrls_->filterPosition,
            &patchCtrls_->filterResonanceModAmount,
            &patchCtrls_->filterResonanceCvAmount,
            &patchCtrls_->filterResonanceRndAmount);

        knobs_[PARAM_KNOB_RESONATOR_TUNE] = KnobController::create(patchState_,
            &patchCtrls_->resonatorTune, &patchCtrls_->resonatorDissonance,
            &patchCtrls_->resonatorTuneModAmount,
            &patchCtrls_->resonatorTuneCvAmount,
            &patchCtrls_->resonatorTuneRndAmount, 0.005f);
        knobs_[PARAM_KNOB_RESONATOR_FEEDBACK] =
            KnobController::create(patchState_, &patchCtrls_->resonatorFeedback,
                NULL, &patchCtrls_->resonatorFeedbackModAmount,
                &patchCtrls_->resonatorFeedbackCvAmount,
                &patchCtrls_->resonatorFeedbackRndAmount);

        knobs_[PARAM_KNOB_ECHO_DENSITY] =
            KnobController::create(patchState_, &patchCtrls_->echoDensity,
                &patchCtrls_->echoFilter, &patchCtrls_->echoDensityModAmount,
                &patchCtrls_->echoDensityCvAmount,
                &patchCtrls_->echoDensityRndAmount, 0.005f);
        knobs_[PARAM_KNOB_ECHO_REPEATS] = KnobController::create(patchState_,
            &patchCtrls_->echoRepeats, NULL, &patchCtrls_->echoRepeatsModAmount,
            &patchCtrls_->echoRepeatsCvAmount,
            &patchCtrls_->echoRepeatsRndAmount);

        knobs_[PARAM_KNOB_AMBIENCE_SPACETIME] = KnobController::create(patchState_,
            &patchCtrls_->ambienceSpacetime, &patchCtrls_->ambienceAutoPan,
            &patchCtrls_->ambienceSpacetimeModAmount,
            &patchCtrls_->ambienceSpacetimeCvAmount,
            &patchCtrls_->ambienceSpacetimeRndAmount, 0.005f);
        knobs_[PARAM_KNOB_AMBIENCE_DECAY] = KnobController::create(patchState_,
            &patchCtrls_->ambienceDecay, NULL, &patchCtrls_->ambienceDecayModAmount,
            &patchCtrls_->ambienceDecayCvAmount,
            &patchCtrls_->ambienceDecayRndAmount);

        knobs_[PARAM_KNOB_MOD_LEVEL] =
            KnobController::create(patchState_, &patchCtrls_->modLevel);
        knobs_[PARAM_KNOB_MOD_SPEED] = KnobController::create(
            patchState_, &patchCtrls_->modSpeed, &patchCtrls_->modType);

        switches_[PARAM_SWITCH_MAP_SELECTOR] =
            SwitchController::create(&patchCtrls_->mapTarget);

        cvs_[PARAM_CV_FILTER_CUTOFF] = CvController::create(
            &patchCvs_->filterCutoff, kCvLpCoeff, kCvOffset, kCvMult, 0.f);
        cvs_[PARAM_CV_FILTER_RESONANCE] =
            CvController::create(&patchCvs_->filterResonance);
        cvs_[PARAM_CV_RESONATOR_TUNE] = CvController::create(
            &patchCvs_->resonatorTune, kCvLpCoeff, kCvOffset, kCvMult, 0.f);
        cvs_[PARAM_CV_RESONATOR_FEEDBACK] =
            CvController::create(&patchCvs_->resonatorFeedback);
        cvs_[PARAM_CV_ECHO_DENSITY] =
            CvController::create(&patchCvs_->echoDensity, 0.995f);
        cvs_[PARAM_CV_ECHO_REPEATS] = CvController::create(&patchCvs_->echoRepeats);
        cvs_[PARAM_CV_AMBIENCE_SPACETIME] =
            CvController::create(&patchCvs_->ambienceSpacetime, 0.995f);
        cvs_[PARAM_CV_AMBIENCE_DECAY] =
            CvController::create(&patchCvs_->ambienceDecay);

        leds_[LED_INPUT] = Led::create(INPUT_LED_PARAM, LedType::LED_TYPE_PARAM);
        leds_[LED_INPUT_PEAK] = Led::create(INPUT_PEAK_LED_PARAM);
        leds_[LED_SYNC] = Led::create(SYNC_IN);
        leds_[LED_MOD] = Led::create(MOD_LED_PARAM, LedType::LED_TYPE_PARAM);
        leds_[LED_RANDOM] = Led::create(RANDOM_BUTTON);
        leds_[LED_MAP] = Led::create(MAP_BUTTON);
        leds_[LED_SHIFT] = Led::create(SHIFT_BUTTON);

        midiOuts_[PARAM_MIDI_FILTER_CUTOFF] = MidiController::create(
            &patchCtrls_->filterCutoff, ParamMidi::PARAM_MIDI_FILTER_CUTOFF);
        midiOuts_[PARAM_MIDI_FILTER_RESONANCE] = MidiController::create(
            &patchCtrls_->filterResonance, ParamMidi::PARAM_MIDI_FILTER_RESONANCE);
        midiOuts_[PARAM_MIDI_FILTER_MODE] = MidiController::create(
            &patchCtrls_->filterMode, ParamMidi::PARAM_MIDI_FILTER_MODE);
        midiOuts_[PARAM_MIDI_FILTER_POSITION] = MidiController::create(
            &patchCtrls_->filterPosition, ParamMidi::PARAM_MIDI_FILTER_POSITION);
        midiOuts_[PARAM_MIDI_FILTER_VOL] = MidiController::create(
            &patchCtrls_->filterVol, ParamMidi::PARAM_MIDI_FILTER_VOL);
        midiOuts_[PARAM_MIDI_RESONATOR_TUNE] = MidiController::create(
            &patchCtrls_->resonatorTune, ParamMidi::PARAM_MIDI_RESONATOR_TUNE);
        midiOuts_[PARAM_MIDI_RESONATOR_FEEDBACK] = MidiController::create(
            &patchCtrls_->resonatorFeedback, ParamMidi::PARAM_MIDI_RESONATOR_FEEDBACK);
        midiOuts_[PARAM_MIDI_RESONATOR_DISSONANCE] =
            MidiController::create(&patchCtrls_->resonatorDissonance,
                ParamMidi::PARAM_MIDI_RESONATOR_DISSONANCE);
        midiOuts_[PARAM_MIDI_RESONATOR_VOL] = MidiController::create(
            &patchCtrls_->resonatorVol, ParamMidi::PARAM_MIDI_RESONATOR_VOL);
        midiOuts_[PARAM_MIDI_ECHO_REPEATS] = MidiController::create(
            &patchCtrls_->echoRepeats, ParamMidi::PARAM_MIDI_ECHO_REPEATS);
        midiOuts_[PARAM_MIDI_ECHO_DENSITY] = MidiController::create(
            &patchCtrls_->echoDensity, ParamMidi::PARAM_MIDI_ECHO_DENSITY);
        midiOuts_[PARAM_MIDI_ECHO_FILTER] = MidiController::create(
            &patchCtrls_->echoFilter, ParamMidi::PARAM_MIDI_ECHO_FILTER);
        midiOuts_[PARAM_MIDI_ECHO_VOL] = MidiController::create(
            &patchCtrls_->echoVol, ParamMidi::PARAM_MIDI_ECHO_VOL);
        midiOuts_[PARAM_MIDI_AMBIENCE_DECAY] = MidiController::create(
            &patchCtrls_->ambienceDecay, ParamMidi::PARAM_MIDI_AMBIENCE_DECAY);
        midiOuts_[PARAM_MIDI_AMBIENCE_SPACETIME] = MidiController::create(
            &patchCtrls_->ambienceSpacetime, ParamMidi::PARAM_MIDI_AMBIENCE_SPACETIME);
        midiOuts_[PARAM_MIDI_AMBIENCE_AUTOPAN] = MidiController::create(
            &patchCtrls_->ambienceAutoPan, ParamMidi::PARAM_MIDI_AMBIENCE_AUTOPAN);
        midiOuts_[PARAM_MIDI_AMBIENCE_VOL] = MidiController::create(
            &patchCtrls_->ambienceVol, ParamMidi::PARAM_MIDI_AMBIENCE_VOL);
        midiOuts_[PARAM_MIDI_MOD_LEVEL] = MidiController::create(
            &patchCtrls_->modLevel, ParamMidi::PARAM_MIDI_MOD_LEVEL);
        midiOuts_[PARAM_MIDI_MOD_SPEED] = MidiController::create(
            &patchCtrls_->modSpeed, ParamMidi::PARAM_MIDI_MOD_SPEED);
        midiOuts_[PARAM_MIDI_MOD_TYPE] = MidiController::create(
            &patchCtrls_->modType, ParamMidi::PARAM_MIDI_MOD_TYPE);
        midiOuts_[PARAM_MIDI_MAP_SELECTOR] = MidiController::create(
            &patchCtrls_->mapTarget, ParamMidi::PARAM_MIDI_MAP_SELECTOR);
        midiOuts_[PARAM_MIDI_RANDOMIZE] =
            MidiController::create(&randomize_, ParamMidi::PARAM_MIDI_RANDOMIZE);

        midiOuts_[PARAM_MIDI_FILTER_CUTOFF_CV] =
            MidiController::create(&patchCvs_->filterCutoff,
                ParamMidi::PARAM_MIDI_FILTER_CUTOFF_CV, 0, 0.5f, 0.6666667f);
        midiOuts_[PARAM_MIDI_RESONATOR_TUNE_CV] =
            MidiController::create(&patchCvs_->resonatorTune,
                ParamMidi::PARAM_MIDI_RESONATOR_TUNE_CV, 0, 0.5f, 0.6666667f);
        midiOuts_[PARAM_MIDI_ECHO_DENSITY_CV] =
            MidiController::create(&patchCvs_->echoDensity,
                ParamMidi::PARAM_MIDI_ECHO_DENSITY_CV, 0, 0.5f, 0.6666667f);
        midiOuts_[PARAM_MIDI_AMBIENCE_SPACETIME_CV] =
            MidiController::create(&patchCvs_->ambienceSpacetime,
                ParamMidi::PARAM_MIDI_AMBIENCE_SPACETIME_CV, 0, 0.5f, 0.6666667f);

        mapButton_ = MapButtonController::create(leds_[LED_MAP]);
        randomButton_ = RandomButtonController::create(leds_[LED_RANDOM]);
        shiftButton_ = ShiftButtonController::create(leds_[LED_SHIFT]);
    }
    ~Ui() {
        FloatArray::destroy(patchState_->outputLevel);
        FloatArray::destroy(patchState_->efModLevel);
        TapTempo::destroy(patchState_->tempo);
        for (size_t i = 0; i < PARAM_KNOB_LAST; i++) {
            KnobController::destroy(knobs_[i]);
        }
        for (size_t i = 0; i < PARAM_FADER_LAST; i++) {
            FaderController::destroy(faders_[i]);
        }
        for (size_t i = 0; i < PARAM_CV_LAST; i++) {
            CvController::destroy(cvs_[i]);
        }
        for (size_t i = 0; i < PARAM_SWITCH_LAST; i++) {
            SwitchController::destroy(switches_[i]);
        }
        for (size_t i = 0; i < LED_LAST; i++) {
            Led::destroy(leds_[i]);
        }
        for (size_t i = 0; i < PARAM_MIDI_LAST; i++) {
            MidiController::destroy(midiOuts_[i]);
        }
        MapButtonController::destroy(mapButton_);
        RandomButtonController::destroy(randomButton_);
        ShiftButtonController::destroy(shiftButton_);
    }

    static Ui* create(
        PatchCtrls* patchCtrls, PatchCvs* patchCvs, PatchState* patchState) {
        return new Ui(patchCtrls, patchCvs, patchState);
    }

    static void destroy(Ui* obj) {
        delete obj;
    }

    void LoadConfig() {
        Resource* resource = Resource::load(PATCH_SETTINGS_NAME ".cfg");
        if (resource) {
            Configuration* configuration = (Configuration*)resource->getData();
            patchState_->modAttenuverters = configuration->mod_attenuverters;
            patchState_->cvAttenuverters = configuration->cv_attenuverters;
            hwRevision_ = configuration->revision;
        }
        else
        {
            // Sensible default values (the same are set in the firmware).
            hwRevision_ = 0;
            patchState_->modAttenuverters = false;
            patchState_->cvAttenuverters = false;
        }
        Resource::destroy(resource);
    }

    void LoadAltParams() {
        Resource* resource = Resource::load(PATCH_SETTINGS_NAME ".alt");
        if (resource != NULL) {
            int16_t* cfg = (int16_t*)resource->getData();
            knobs_[PARAM_KNOB_AMBIENCE_SPACETIME]->SetValue(cfg[0] / 8192.f, LockableParamName::PARAM_LOCKABLE_ALT); // Ambience auto-pan
            knobs_[PARAM_KNOB_ECHO_DENSITY]->SetValue(cfg[1] / 8192.f, LockableParamName::PARAM_LOCKABLE_ALT); // Echo filter
            knobs_[PARAM_KNOB_FILTER_RESONANCE]->SetValue(cfg[2] / 8192.f, LockableParamName::PARAM_LOCKABLE_ALT); // Filter position
            knobs_[PARAM_KNOB_FILTER_CUTOFF]->SetValue(cfg[3] / 8192.f, LockableParamName::PARAM_LOCKABLE_ALT); // Filter mode
            knobs_[PARAM_KNOB_MOD_SPEED]->SetValue(cfg[4] / 8192.f, LockableParamName::PARAM_LOCKABLE_ALT); // Mod type
            knobs_[PARAM_KNOB_RESONATOR_TUNE]->SetValue(cfg[5] / 8192.f, LockableParamName::PARAM_LOCKABLE_ALT); // Reso dissonance
        }
        Resource::destroy(resource);
    }

    void LoadModParams() {
        Resource* resource = Resource::load(PATCH_SETTINGS_NAME ".mod");
        if (resource) {
            int16_t* cfg = (int16_t*)resource->getData();
            knobs_[PARAM_KNOB_AMBIENCE_DECAY]->SetValue(cfg[0] / 8192.f, LockableParamName::PARAM_LOCKABLE_MOD); // Ambience decay mod amount
            knobs_[PARAM_KNOB_AMBIENCE_SPACETIME]->SetValue(cfg[1] / 8192.f, LockableParamName::PARAM_LOCKABLE_MOD); // Ambience spacetime mod amount
            knobs_[PARAM_KNOB_ECHO_DENSITY]->SetValue(cfg[2] / 8192.f, LockableParamName::PARAM_LOCKABLE_MOD); // Echo density mod amount
            knobs_[PARAM_KNOB_ECHO_REPEATS]->SetValue(cfg[3] / 8192.f, LockableParamName::PARAM_LOCKABLE_MOD); // Echo repeats mod amount
            knobs_[PARAM_KNOB_FILTER_CUTOFF]->SetValue(cfg[4] / 8192.f, LockableParamName::PARAM_LOCKABLE_MOD); // Filter cutoff mod amount
            knobs_[PARAM_KNOB_FILTER_RESONANCE]->SetValue(cfg[5] / 8192.f, LockableParamName::PARAM_LOCKABLE_MOD); // Filter resonance mod amount
            knobs_[PARAM_KNOB_RESONATOR_FEEDBACK]->SetValue(cfg[6] / 8192.f, LockableParamName::PARAM_LOCKABLE_MOD); // Resonator feedback mod amount
            knobs_[PARAM_KNOB_RESONATOR_TUNE]->SetValue(cfg[7] / 8192.f, LockableParamName::PARAM_LOCKABLE_MOD); // Resonator tune mod amount
        }
        Resource::destroy(resource);
    }

    void LoadCvParams() {
        Resource* resource = Resource::load(PATCH_SETTINGS_NAME ".cv");
        if (resource) {
            int16_t* cfg = (int16_t*)resource->getData();
            knobs_[PARAM_KNOB_AMBIENCE_DECAY]->SetValue(cfg[0] / 8192.f, LockableParamName::PARAM_LOCKABLE_CV); // Ambience decay cv amount
            knobs_[PARAM_KNOB_AMBIENCE_SPACETIME]->SetValue(cfg[1] / 8192.f, LockableParamName::PARAM_LOCKABLE_CV); // Ambience spacetime cv amount
            knobs_[PARAM_KNOB_ECHO_DENSITY]->SetValue(cfg[2] / 8192.f, LockableParamName::PARAM_LOCKABLE_CV); // Echo density cv amount
            knobs_[PARAM_KNOB_ECHO_REPEATS]->SetValue(cfg[3] / 8192.f, LockableParamName::PARAM_LOCKABLE_CV); // Echo repeats cv amount
            knobs_[PARAM_KNOB_FILTER_CUTOFF]->SetValue(cfg[4] / 8192.f, LockableParamName::PARAM_LOCKABLE_CV); // Filter cutoff cv amount
            knobs_[PARAM_KNOB_FILTER_RESONANCE]->SetValue(cfg[5] / 8192.f, LockableParamName::PARAM_LOCKABLE_CV); // Filter resonance cv amount
            knobs_[PARAM_KNOB_RESONATOR_FEEDBACK]->SetValue(cfg[6] / 8192.f, LockableParamName::PARAM_LOCKABLE_CV); // Resonator feedback cv amount
            knobs_[PARAM_KNOB_RESONATOR_TUNE]->SetValue(cfg[7] / 8192.f, LockableParamName::PARAM_LOCKABLE_CV); // Resonator tune cv amount
        }
        Resource::destroy(resource);
    }

    void LoadRndParams() {
        Resource* resource = Resource::load(PATCH_SETTINGS_NAME ".rnd");
        if (resource) {
            int16_t* cfg = (int16_t*)resource->getData();
            knobs_[PARAM_KNOB_AMBIENCE_DECAY]->SetValue(cfg[0] / 8192.f, LockableParamName::PARAM_LOCKABLE_RND); // Ambience decay random amount
            knobs_[PARAM_KNOB_AMBIENCE_SPACETIME]->SetValue(cfg[1] / 8192.f, LockableParamName::PARAM_LOCKABLE_RND); // Ambience spacetime random amount
            knobs_[PARAM_KNOB_ECHO_DENSITY]->SetValue(cfg[2] / 8192.f, LockableParamName::PARAM_LOCKABLE_RND); // Echo density random amount
            knobs_[PARAM_KNOB_ECHO_REPEATS]->SetValue(cfg[3] / 8192.f, LockableParamName::PARAM_LOCKABLE_RND); // Echo repeats random amount
            knobs_[PARAM_KNOB_FILTER_CUTOFF]->SetValue(cfg[4] / 8192.f, LockableParamName::PARAM_LOCKABLE_RND); // Filter cutoff random amount
            knobs_[PARAM_KNOB_FILTER_RESONANCE]->SetValue(cfg[5] / 8192.f, LockableParamName::PARAM_LOCKABLE_RND); // Filter resonance random amount
            knobs_[PARAM_KNOB_RESONATOR_FEEDBACK]->SetValue(cfg[6] / 8192.f, LockableParamName::PARAM_LOCKABLE_RND); // Resonator feedback random amount
            knobs_[PARAM_KNOB_RESONATOR_TUNE]->SetValue(cfg[7] / 8192.f, LockableParamName::PARAM_LOCKABLE_RND); // Resonator tune random amount
        }
        Resource::destroy(resource);
    }

    void LoadMainParams() {
        Resource* resource = Resource::load(PATCH_SETTINGS_NAME ".prm");
        if (resource) {
            int16_t* cfg = (int16_t*)resource->getData();
            knobs_[PARAM_KNOB_AMBIENCE_DECAY]->SetValue(cfg[0] / 8192.f); // Ambience decay
            knobs_[PARAM_KNOB_AMBIENCE_SPACETIME]->SetValue(cfg[1] / 8192.f); // Ambience spacetime
            knobs_[PARAM_KNOB_ECHO_DENSITY]->SetValue(cfg[2] / 8192.f); // Echo density
            knobs_[PARAM_KNOB_ECHO_REPEATS]->SetValue(cfg[3] / 8192.f); // Echo repeats
            knobs_[PARAM_KNOB_FILTER_CUTOFF]->SetValue(cfg[4] / 8192.f); // Filter cutoff
            knobs_[PARAM_KNOB_FILTER_RESONANCE]->SetValue(cfg[5] / 8192.f); // Filter resonance
            knobs_[PARAM_KNOB_RESONATOR_FEEDBACK]->SetValue(cfg[6] / 8192.f); // Resonator feedback
            knobs_[PARAM_KNOB_RESONATOR_TUNE]->SetValue(cfg[7] / 8192.f); // Resonator tune
            knobs_[PARAM_KNOB_MOD_LEVEL]->SetValue(cfg[8] / 8192.f); // Mod level
            knobs_[PARAM_KNOB_MOD_SPEED]->SetValue(cfg[9] / 8192.f); // Mod speed
        }
        Resource::destroy(resource);
    }

    void SaveParametersConfig(FuncMode funcMode) {
        /*
        if (!parameterChangedSinceLastSave_)
        {
            return;
        }
        */

        // saving_ = true;
        // parameterChangedSinceLastSave_ = false;

        float values[MAX_PATCH_SETTINGS] {};

        switch (funcMode) {
        case FUNC_MODE_ALT:
            values[0] = patchCtrls_->ambienceAutoPan;
            values[1] = patchCtrls_->echoFilter;
            values[2] = patchCtrls_->filterPosition;
            values[3] = patchCtrls_->filterMode;
            values[4] = patchCtrls_->modType;
            values[5] = patchCtrls_->resonatorDissonance;
            break;
        case FUNC_MODE_MOD:
            values[0] = patchCtrls_->ambienceDecayModAmount;
            values[1] = patchCtrls_->ambienceSpacetimeModAmount;
            values[2] = patchCtrls_->echoDensityModAmount;
            values[3] = patchCtrls_->echoRepeatsModAmount;
            values[4] = patchCtrls_->filterCutoffModAmount;
            values[5] = patchCtrls_->filterResonanceModAmount;
            values[6] = patchCtrls_->resonatorFeedbackModAmount;
            values[7] = patchCtrls_->resonatorTuneModAmount;
            break;
        case FUNC_MODE_CV:
            values[0] = patchCtrls_->ambienceDecayCvAmount;
            values[1] = patchCtrls_->ambienceSpacetimeCvAmount;
            values[2] = patchCtrls_->echoDensityCvAmount;
            values[3] = patchCtrls_->echoRepeatsCvAmount;
            values[4] = patchCtrls_->filterCutoffCvAmount;
            values[5] = patchCtrls_->filterResonanceCvAmount;
            values[6] = patchCtrls_->resonatorFeedbackCvAmount;
            values[7] = patchCtrls_->resonatorTuneCvAmount;
            break;
        case FUNC_MODE_RND:
            values[0] = patchCtrls_->ambienceDecayRndAmount;
            values[1] = patchCtrls_->ambienceSpacetimeRndAmount;
            values[2] = patchCtrls_->echoDensityRndAmount;
            values[3] = patchCtrls_->echoRepeatsRndAmount;
            values[4] = patchCtrls_->filterCutoffRndAmount;
            values[5] = patchCtrls_->filterResonanceRndAmount;
            values[6] = patchCtrls_->resonatorFeedbackRndAmount;
            values[7] = patchCtrls_->resonatorTuneRndAmount;
            break;

        default:
            // NONE
            values[0] = patchCtrls_->ambienceDecay;
            values[1] = patchCtrls_->ambienceSpacetime;
            values[2] = patchCtrls_->echoDensity;
            values[3] = patchCtrls_->echoRepeats;
            values[4] = patchCtrls_->filterCutoff;
            values[5] = patchCtrls_->filterResonance;
            values[6] = patchCtrls_->resonatorFeedback;
            values[7] = patchCtrls_->resonatorTune;
            values[8] = patchCtrls_->modLevel;
            values[9] = patchCtrls_->modSpeed;
            break;
        }

        // Start the save process.
        getInitialisingPatchProcessor()->patch->sendMidi(MidiMessage(USB_COMMAND_SINGLE_BYTE, START, 0, 0)); // send MIDI START

        // Send the file index - 0: "iroi.prm", 1: "iroi.alt", 2: "iroi.mod", 3: "iroi.cv", 4: "iroi.rnd"
        getInitialisingPatchProcessor()->patch->sendMidi(MidiMessage::cp(0, funcMode));

        for (size_t i = 0; i < MAX_PATCH_SETTINGS; i++) 
        {
            // Convert to 14-bit (16384) signed int.
            int16_t value = rintf(values[i] * 8192);
            // Send the parameter's value.
            getInitialisingPatchProcessor()->patch->sendMidi(MidiMessage::pb(i, value));
        }

        // Finish the process.
        getInitialisingPatchProcessor()->patch->sendMidi(MidiMessage(USB_COMMAND_SINGLE_BYTE, STOP, 0, 0)); // send MIDI STOP
    }

    // Callback
    void ProcessButton(PatchButtonId bid, uint16_t value, uint16_t samples) {
        bool on = value != 0;

        switch (bid) {
        case SYNC_IN:
            patchState_->tempo->trigger(on, samples);
            patchState_->syncIn = on;
            break;

        case RANDOM_IN:
            if (on) {
                leds_[LED_RANDOM]->Blink();
                if (!randomize_) {
                    patchState_->randomSlew = 1.f / kRandomSlewSamples;
                    patchState_->randomHasSlew = false;
                    randomize_ = true;
                }
            }
            break;

        case RANDOM_BUTTON:
            randomButton_->Trig(on);
            break;

        case MAP_BUTTON:
            mapButton_->Trig(on);
            break;

        case SHIFT_BUTTON:
            shiftButton_->Trig(on);
            break;

        case IN_DETEC:
            break;

        default:
            break;
        }
    }

    // Callback.
    void ProcessMidi(MidiMessage msg) {
        return;

        if (msg.isControlChange()) {
            if (msg.getControllerNumber() < PARAM_MIDI_LAST) {
                // midiOuts_[msg.getControllerNumber()]->SetValue(msg.getControllerValue() / 127.0f);
            }
        }
        /*
        else if(msg.isNoteOn())
        {
            midinote = msg.getNote();
        }
        */
    }

    void HandleLeds() {
        float level = patchState_->outputLevel.getMaxValue();
        if (level < 0.6f) {
            leds_[LED_INPUT]->Set(Map(level, 0.f, 0.6f, 0.45f, 1.f));
            leds_[LED_INPUT_PEAK]->Off();
        }
        else {
            leds_[LED_INPUT]->Off();
            leds_[LED_INPUT_PEAK]->On();
        }

        float v = Map(patchState_->modValue, -0.5f, 0.5f, 0.49f, 0.5f + patchCtrls_->modLevel * 0.5f);
        if (v < 0.5f)
        {
            v = 0;
        }

        leds_[LED_MOD]->Set(v);

        if (patchState_->clockTick) {
            leds_[LED_SYNC]->Blink();
        }

        if (modTypeLockTrigger_.Process(patchState_->modTypeLockFlag) ||
            modSpeedLockTrigger_.Process(patchState_->modSpeedLockFlag) ||
            filterModeTrigger_.Process(patchState_->filterModeFlag) ||
            filterPositionTrigger_.Process(patchState_->filterPositionFlag)) {
            leds_[LED_SHIFT]->Blink(2, true);
        }
    }

    void HandleCatchUp() {
        bool moving = false;
        bool wasCatchingUp = false;

        for (size_t i = 0; i < PARAM_KNOB_LAST + PARAM_FADER_LAST; i++) {
            CatchUpController* ctrl = (i < PARAM_KNOB_LAST)
                ? (CatchUpController*)knobs_[i]
                : (CatchUpController*)faders_[i - PARAM_KNOB_LAST];
            bool m = ctrl->Process();
            patchState_->moving[i] = m;
            if (m && !moving) {
                movingParam_ = ctrl;
                moving = true;
            }
        }
    }
    
    void HandleLedButtons() {
        mapButton_->Process();
        randomButton_->Process();
        shiftButton_->Process();

        mapActive_ = mapButton_->IsOn() && !mapButton_->IsPressed() && !shiftButton_->IsOn();

        FuncMode funcMode = patchState_->funcMode;

        if (mapButton_->IsPressed() && !shiftButton_->IsOn()) {
            // Handle long press for saving when the shift
            // button is not pressed.
            if (samplesSinceMapButtonPressed_ < kSaveLimit) {
                samplesSinceMapButtonPressed_++;
                leds_[LED_MAP]->On();
            }
            else if (!saveFlag_) {
                // Save.
                saveFlag_ = true;
                saving_ = true;
                fadeOutOutput_ = true;
                leds_[LED_MAP]->Off();
            }
        }
        else if (shiftButton_->IsOn()) {
            if (mapButton_->IsOn() && !mapButtonWasOn_ && patchState_->funcMode != FUNC_MODE_ALT)
            {
                // Temporarily exit mapping.
                mapButtonWasOn_ = true;
                mapButton_->Set(0);
            }

            // Only SHIFT button is on.
            if (saveFlag_) {
                // Saving button has been released.
                samplesSinceMapButtonPressed_ = 0;
                saveFlag_ = false;
            }
            else {
                funcMode = FuncMode::FUNC_MODE_ALT;
            }
        }
        else if (!shiftButton_->IsOn() && !mapButton_->IsOn()) {
            // Neither SHIFT nor MAP_BUTTON buttons are on.
            if (saveFlag_) {
                // Saving button has been released.
                saveFlag_ = false;
            }
            else {
                funcMode = FuncMode::FUNC_MODE_NONE;
            }
            samplesSinceMapButtonPressed_ = 0;

            if (mapButtonWasOn_)
            {
                // If mapping was temporarily disabled when SHIFT
                // button was pressed, re-enable it.
                mapButtonWasOn_ = false;
                mapButton_->Set(1);
            }
        }

        if (mapActive_) {
            funcMode = selectedMapTarget_;
        }

        if (funcMode != patchState_->funcMode) {
            patchState_->funcMode = funcMode;
            for (size_t i = 0; i < PARAM_KNOB_LAST; i++) {
                knobs_[i]->SetFuncMode(patchState_->funcMode);
            }
            mapButton_->SetFuncMode(patchState_->funcMode);
            randomButton_->SetFuncMode(patchState_->funcMode);
        }

        if (patchState_->funcMode != FuncMode::FUNC_MODE_ALT) 
        {
            // Handle long press of the random button for slewing.
            if (randomButton_->IsPressed()) {
                leds_[LED_RANDOM]->On();
                samplesSinceRandomPressed_++;
            }
            else if (samplesSinceRandomPressed_ > 0 && !randomize_) {
                // Random button has been released.
                doRandomSlew_ = samplesSinceRandomPressed_ > kRandomSlewSamples;
                patchState_->randomHasSlew = doRandomSlew_;
                patchState_->randomSlew = 1.f / samplesSinceRandomPressed_;
                randomSlewInc_ = 0;
                samplesSinceRandomPressed_ = 0;
                randomize_ = true;
            }
        }
        else
        {
            if (mapButton_->IsPressed() || randomButton_->IsPressed()) 
            {
                if (samplesSinceMapOrRandomPressed_ < kResetLimit) {
                    samplesSinceMapOrRandomPressed_++;
                }
                else if (mapAndRandomTrigger_.Process(mapButton_->IsPressed() && randomButton_->IsPressed())) {
                    mapAndRandomPressed_ = true;
                    // Reset parameters.
                    for (size_t i = 0; i < PARAM_KNOB_LAST; i++) {
                        knobs_[i]->Reset(FUNC_MODE_NONE);
                    }
                }
                // mapAndRandomPressed_ assures that if the last operation
                // was the reset of parameters, the next operation may be
                // performed only if the buttons are released.
                else if (!mapAndRandomPressed_) {
                    if (FuncMode::FUNC_MODE_ALT == patchState_->funcMode) {
                        if (!undoRedo_ && randomButton_->IsOn()) {
                            undoRedo_ = true;
                        }
                        else if (mapButton_->IsOn()) {
                            // Reset selected mappings.
                            for (size_t i = 0; i < PARAM_KNOB_LAST; i++) {
                                knobs_[i]->Reset(selectedMapTarget_);
                            }
                        }
                    }
                }
            }
            else {
                samplesSinceMapOrRandomPressed_ = 0;
                mapAndRandomPressed_ = false;
                mapAndRandomTrigger_.Process(0);
                undoRedoRandomTrigger_.Process(0);
            }
        }
    }
    
    void UndoRedo() {
        knobs_[PARAM_KNOB_FILTER_CUTOFF]->UndoRedo();
        knobs_[PARAM_KNOB_FILTER_RESONANCE]->UndoRedo();
        
        knobs_[PARAM_KNOB_RESONATOR_TUNE]->UndoRedo();
        knobs_[PARAM_KNOB_RESONATOR_FEEDBACK]->UndoRedo();
        
        knobs_[PARAM_KNOB_ECHO_REPEATS]->UndoRedo();
        knobs_[PARAM_KNOB_ECHO_DENSITY]->UndoRedo();
        
        knobs_[PARAM_KNOB_AMBIENCE_DECAY]->UndoRedo();
        knobs_[PARAM_KNOB_AMBIENCE_SPACETIME]->UndoRedo();

        undoRedo_ = false;
    }

    void Randomize() {
        knobs_[PARAM_KNOB_FILTER_CUTOFF]->Randomize();
        knobs_[PARAM_KNOB_FILTER_RESONANCE]->Randomize();

        knobs_[PARAM_KNOB_RESONATOR_TUNE]->Randomize();
        knobs_[PARAM_KNOB_RESONATOR_FEEDBACK]->Randomize();

        knobs_[PARAM_KNOB_ECHO_REPEATS]->Randomize();
        knobs_[PARAM_KNOB_ECHO_DENSITY]->Randomize();

        knobs_[PARAM_KNOB_AMBIENCE_DECAY]->Randomize();
        knobs_[PARAM_KNOB_AMBIENCE_SPACETIME]->Randomize();

        randomize_ = false;
    }

    // Called at block rate
    void Poll() {
        if (startup_) {
            //LoadMainParams();
            LoadAltParams();
            LoadModParams();
            LoadCvParams();
            LoadRndParams();

            startup_ = false;

            return;
        }

        for (size_t i = 0; i < PARAM_KNOB_LAST; i++) {
            knobs_[i]->Read(ParamKnob(i));
        }

        for (size_t i = 0; i < PARAM_FADER_LAST; i++) {
            faders_[i]->Read(ParamFader(i));
        }
        
        for (size_t i = 0; i < PARAM_SWITCH_LAST; i++) {
            switches_[i]->Read(ParamSwitch(i));
        }

        for (size_t i = 0; i < PARAM_CV_LAST; i++) {
            cvs_[i]->Read(ParamCv(i));
        }
        
        for (size_t i = 0; i < LED_LAST; i++) {
            leds_[i]->Read();
        }
        
        for (size_t i = 0; i < PARAM_MIDI_LAST; i++) {
            midiOuts_[i]->Process();
        }

        HandleLeds();
        HandleCatchUp();
        HandleLedButtons();

        // Value: 0 = CV, 0.33 = RND, 1.00 = MOD
        selectedMapTarget_ = FuncMode::FUNC_MODE_CV;
        if (patchCtrls_->mapTarget > 0.9f) {
            selectedMapTarget_ = FuncMode::FUNC_MODE_MOD;
        }
        else if (patchCtrls_->mapTarget > 0.2f) {
            selectedMapTarget_ = FuncMode::FUNC_MODE_RND;
        }

        patchState_->modActive = patchCtrls_->modLevel > 0.1f;

        float cutoffNote = 127 * patchCvs_->filterCutoff * patchCtrls_->filterCutoffCvAmount;
        float interval = cutoffNote - cutoffCv_;
        if (interval < -0.4f || interval > 0.4f) {
            cutoffCv_ = cutoffNote;
        }
        else {
            cutoffCv_ += 0.1f * interval;
        }

        cutoffNote = Modulate(cutoff_, patchCtrls_->filterCutoffModAmount, patchState_->modValue, 0, 0, -1.f, 1.f, patchState_->modAttenuverters);
        cutoffNote = 12 + 115 * cutoffNote;
        cutoffPot_ += 0.1f * (cutoffNote - cutoffPot_);
        patchCtrls_->filterCutoff = Clamp(cutoffPot_ + cutoffCv_, 0.f, 127.f);

        if (patchCtrls_->filterVol >= kOne) {
            patchCtrls_->filterVol = 1.f;
        }
        if (patchCtrls_->resonatorVol >= kOne) {
            patchCtrls_->resonatorVol = 1.f;
        }
        if (patchCtrls_->echoVol >= kOne) {
            patchCtrls_->echoVol = 1.f;
        }
        if (patchCtrls_->ambienceVol >= kOne) {
            patchCtrls_->ambienceVol = 1.f;
        }

        if (fadeOutOutput_) {
            patchState_->outLevel -= kOutputFadeInc;
            if (patchState_->outLevel <= 0) {
                patchState_->outLevel = 0;
                if (saving_) {
                    // Do not save parameters.
                    //SaveParametersConfig(FuncMode::FUNC_MODE_NONE);
                    SaveParametersConfig(FuncMode::FUNC_MODE_ALT);
                    SaveParametersConfig(FuncMode::FUNC_MODE_MOD);
                    SaveParametersConfig(FuncMode::FUNC_MODE_CV);
                    SaveParametersConfig(FuncMode::FUNC_MODE_RND);
                    leds_[LED_MAP]->Blink(2, false, !leds_[LED_MAP]->IsOn());
                    fadeOutOutput_ = false;
                    fadeInOutput_ = true;
                    saving_ = false;
                }
                else {
                    fadeOutOutput_ = false;
                    fadeInOutput_ = true;
                }
            }
        }

        if (fadeInOutput_) {
            patchState_->outLevel += kOutputFadeInc;
            if (patchState_->outLevel >= 1) {
                patchState_->outLevel = 1;
                fadeInOutput_ = false;
            }
        }

        if (randomize_) {
            Randomize();
        }

        if (undoRedo_) {
            UndoRedo();
        }

        if (doRandomSlew_) {
            randomSlewInc_ += patchState_->randomSlew;
            if (randomSlewInc_ >= 1.f) {
                doRandomSlew_ = false;
                leds_[LED_RANDOM]->Blink(2);
            }
        }
    }
};
