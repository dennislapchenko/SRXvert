#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "AcidHeader.h"

using namespace AcidR;

class MainContentComponent   :  public Component,               //main superclass for displaying stuff in main window
                                public FileDragAndDropTarget,   //class to implement file drag and drop callbacks
                                public Button::Listener,        //button callbacks
                                public ComboBox::Listener,      //dropdown menu callbacks
                                public Slider::Listener         //slider callbacks
{
public:
    MainContentComponent()
    {
        centreWithSize(400, 650);               //size of main window
        initializeGUI();                        //setup all buttons and menus
        formatManager.registerBasicFormats();   //register audio file formats to instantiate an appropriate file reader later
        
        //automatedTesting();                   //instant options fill for testing
    }

    ~MainContentComponent()
    {
        shutdownComponent();
    }
    
    void automatedTesting()
    {
        File newFile("../TestData/te/GLASS.wav");
        dragAndDroppedFiles.add(newFile);
        openedFiles = &dragAndDroppedFiles;
        checkIfLoadedFilesValid();
        sampleRateMenu.setSelectedId(44100);
        bitRateMenu.setSelectedId(16);
        appendNameMenu.setSelectedId(1);
        interpolationMenu.setSelectedId(4);
        envelopeMenu.setSelectedId(3);
        newFileLength.setValue(100);
        variStretchSlider.setMinAndMaxValues(0.1, 3.4);
    }
    
    void openButtonClicked()
    {
        if(chooser->browseForMultipleFilesToOpen())     //choose files
        {
            dragAndDroppedFiles = chooser->getResults();//dump results to different location, incase chooser is used to browse for save directory
            openedFiles = &dragAndDroppedFiles;         //assign a pointer, which will later be used to read opened files
        }
        if(openedFiles == nullptr)                      //to avoid an exception if user presses 'Cancel' in the file browsing context
        {
            return;
        }
        
        setGeneralButtonsEnabled(checkIfLoadedFilesValid()); //checks if files are valid and returns true/false which enables CONVERT button
    }
    
    void convertButtonClicked()
    {
        convertButton.setEnabled(false); //disable the button to prevent further clicks until processing is complete
        
        ScopedPointer<WavAudioFormat> wav = new WavAudioFormat();   //wav format class to instantiate appropriate wav writer
        
        for (const File loadedFile : *openedFiles)                  //loops through every opened file, reads, processes and writes out
        {
            AudioFormatReader* reader = formatManager.createReaderFor(loadedFile); //creates a wav file reader for currently opened file from format manager
            
            if(reader != nullptr) //if reader was successfully assigned
            {
                AcidR::AcidAudioBuffer readBuffer; //buffer to read the files data into
                readBuffer.setSampleRate(reader->sampleRate);
                readBuffer.setSize(numChannels, reader->lengthInSamples);
                readBuffer.WriteFromReader(reader); //FILLS BUFFER FROM FILE READERS INPUT STREAM
                if(numChannels < 2) //if was converted to mono - normalise to bring back levels (no hate please)
                {
                    readBuffer.PeakNormalise(1.0f, 0.01f); //max gain and threshold(to avoid raising noise floor)
                }
                
                AcidR::AcidAudioBuffer  granulatedBuffer,   //buffer for writing granulation output
                                        resampledBuffer;    //buffer for writing resampling output
                
                //RESAMPLE IF SELECTED BY USER
                if(shouldResample.getToggleState())
                {
                    if(writeConsoleProgress) DBG("Resampling...");
                    AcidR::AcidResampler resampler;
                    resampledBuffer.setSampleRate(newSampleRate);
                    resampler.resample(&readBuffer, &resampledBuffer, newSampleRate, interp);
                    if(writeConsoleProgress) DBG("Finished resampling");
                }
                
                //GRANULATE IF SELECTED BY USER
                if(shouldGranulate.getToggleState())
                {
                    if(writeConsoleProgress) DBG("Granulating...");
                    if(!shouldResample.getToggleState()) //if didn't resample - get SR, bitrate, channels info from loaded file, rather than user UI input
                    {
                        newSampleRate = reader->sampleRate;
                        numChannels = reader->numChannels;
                        newBitRate = reader->bitsPerSample;
                    }
                    
                    AcidR::AcidGranulator granulator;
                    granulator.setSampleRate(newSampleRate);
                    
                    //set all user defined granulation settings
                    granulator.setGrainSize((int)variGrainSlider.getValue());
                    granulator.setBiasGrain((int)variGrainSlider.getValue());
                    granulator.setRandomGrainSize((int)variGrainSlider.getMinValue(), (int)variGrainSlider.getMaxValue());
                    granulator.setStretch((float)variStretchSlider.getValue());
                    granulator.setBiasStretch((float)variStretchSlider.getValue());
                    granulator.setRandomStretch((float)variStretchSlider.getMinValue(), (float)variStretchSlider.getMaxValue());
                    granulator.setVoices((int)voicesSlider.getValue());
                    granulator.setEnvelopeType(envelopeMenu.getSelectedId());
                    
                    for(int newFilesToWrite = 1; newFilesToWrite <= numOutFilesSlider.getValue(); newFilesToWrite++) //writes however many files user has selected
                    {
                        granulatedFileCounter = newFilesToWrite; //global variable to append to new files names
                        String outputPath = buildOutputPath(loadedFile.getFileName()); //get original path and append settings to the name
                        File outputFile = File(outputPath); //create new file to be written
                        outputFile.deleteFile();            //safecheck to delete the existing file before writing new one
                        
                        ScopedPointer<FileOutputStream> outStream (outputFile.createOutputStream()); //stream that is used by writer internally to write data form buffers
                        ScopedPointer<AudioFormatWriter> writer (wav->createWriterFor(outStream, newSampleRate, numChannels, newBitRate, reader->metadataValues, 0));
                        
                        if(!shouldResample.getToggleState()) //if didnt resample, granulate from file reader buffer
                        {
                            granulatedBuffer.setSampleRate(readBuffer.getSampleRate());
                            granulator.process(&readBuffer, &granulatedBuffer, newFileLength.getValue());
                        }
                        else
                        { //if resampled, granulate from resampled buffer
                            granulatedBuffer.setSampleRate(resampledBuffer.getSampleRate());
                            granulator.process(&resampledBuffer, &granulatedBuffer, newFileLength.getValue());
                        }
                        if(writeConsoleProgress) DBG("Finished granulating");
                        if(writeConsoleProgress) DBG("Writing to disk...");
                        writer->writeFromAudioSampleBuffer(granulatedBuffer, 0, granulatedBuffer.getNumSamples()); //write to file from granulated buffer
                        if(writeConsoleProgress) DBG("Finished writing to disk\n-");
                        granulatedBuffer.clear(); //clears to write next file if there is one to write!
                        outStream.release(); //reset output stream for next file
                        
                        outputFile.revealToUser(); //open containing folder with converted files
                    
                    } //multiple granulated files loop
                    resampledBuffer.clear(); //reset resampled buffer only when multiple INPUT files are present, rather than rendering multiple from one input
                }//granulation scope
                
                if(shouldResample.getToggleState() && !shouldGranulate.getToggleState()) //write from resample if didn't granulate and use its writer
                {
                    String outputPath = buildOutputPath(loadedFile.getFileName()); //get original path and append settings to the name
                    File outputFile = File(outputPath); //create new file to be written
                    outputFile.deleteFile();            //safecheck to delete the existing file before writing new one
                    
                    ScopedPointer<FileOutputStream> outStream (outputFile.createOutputStream()); //stream that is used by writer internally to write data form buffers
                    ScopedPointer<AudioFormatWriter> writer (wav->createWriterFor(outStream, newSampleRate, numChannels, newBitRate, reader->metadataValues, 0));
                    if(writeConsoleProgress) DBG("Writing to disk...");
                    writer->writeFromAudioSampleBuffer(resampledBuffer, 0, resampledBuffer.getNumSamples()); //writes to file
                    if(writeConsoleProgress) DBG("Finished writing to disk");
                    outStream.release(); //reset output stream for next file
                    outputFile.revealToUser(); //open containing folder with converted files
                }
                
                resampledBuffer.clear();
                readBuffer.clear();
                delete reader; //delete reader from memory so that a new one for next file could be created
            }
            
            if(deleteOriginalsToggle.getToggleState() && appendNameMenu.getSelectedId() != 4) //deletes the original files if selected by the user
            {
                loadedFile.deleteFile();
            }
        }//multiple file convert loop
        queuedFileCount.setColour(TextEditor::textColourId, Colours::green);
        queuedFileCount.setText(String(openedFiles->size())+ " " +(openedFiles->size() == 1 ? "file" : "files") + " successfully processed!"); //update GUI
        
        convertButton.setEnabled(true); //re-enable button after everything is processed
    } //"convertbuttonclicked" end

    void stereoToggleClicked(bool toggled)
    {
        if(toggled) numChannels = 2;
        else        numChannels = 1;
    }
    
    String buildOutputPath(String fileName)
    {
        String result = outputFolder +"/"+ fileName.trimCharactersAtEnd(".wav");
        if(shouldGranulate.getToggleState())
        {
            result += "_gran("+String(granulatedFileCounter)+")";
            return result += ".wav";
        }
        else //if resampling only
        {
            switch(appendNameMenu.getSelectedId())
            {
                case(1):
                    result += "_"+String(newSampleRate);
                    break;
                case(2):
                    result += "_"+String(newBitRate);
                    break;
                case(3):
                    result += "_"+String(newSampleRate)+"_"+String(newBitRate);
                    break;
                default:
                    break;
            }
            return result += ".wav";
        }
        return result += "unexpected_naming_outcome";
    }
    
    //callback - dragged files are hovered over application window
    bool isInterestedInFileDrag(const StringArray& files) override
    {
        openButton.setColour(TextButton::buttonColourId, Colours::green);
        return true;
    }
    //callback - dragged files leave applications space
    void fileDragExit (const StringArray& files) override
    {
        openButton.setColour(TextButton::buttonColourId, Colours::white);
    }
    
    //callback, when dragged files are dropped. reads their absolute path and instantiates array with those files
    void filesDropped (const StringArray& files, int x, int y) override
    {
        openButton.setColour(TextButton::buttonColourId, Colours::white);
        dragAndDroppedFiles.clear();
        if(files.size() != 0)
        {
            for(const String path : files)
            {
                File newFile(path);
                dragAndDroppedFiles.add(newFile);
            }
   
            openedFiles = &dragAndDroppedFiles;
            setGeneralButtonsEnabled(checkIfLoadedFilesValid());
        }
    }
    
    bool checkIfLoadedFilesValid()
    {
        if(openedFiles->getFirst().hasFileExtension("wav") || openedFiles->getFirst().hasFileExtension("mp3") || openedFiles->getFirst().hasFileExtension("aif") || openedFiles->getFirst().hasFileExtension("aiff"))
        {
            queuedFileCount.setColour(TextEditor::textColourId, Colours::green);
            queuedFileCount.setText(String(openedFiles->size())+ " " +(openedFiles->size() == 1 ? "file" : "files") + " queued for processing"); //update GUI
            
            outputFolder = openedFiles->getFirst().getParentDirectory().getFullPathName();
            outputDirectory.setText(outputFolder);
            return true;
        }
        else
        {   //In case invalid files were opened
            queuedFileCount.setColour(TextEditor::textColourId, Colours::red);
            queuedFileCount.setText("Invalid data. Only *.wav, *.mp3 & *.aiff are accepted.");
            openedFiles = nullptr;
            return false;
        }
    }
    
    void shutdownComponent()
    {
    }

//===GUI===============================================================================================================
    /* Every UI element is: • Declared
                            • Initialized
                            • Given position & dimensions
                            • Listener callback configured
     */
     
    void initializeGUI()
    {
        //WINDOW CLOSE BUTTON
        addAndMakeVisible(&windowCloseButton);
        windowCloseButton.setButtonText("X");
        windowCloseButton.setEnabled(true);
        windowCloseButton.addListener(this);
        windowCloseButton.setColour(TextButton::buttonColourId, Colours::indianred);
        
        //OPEN BUTTON
        addAndMakeVisible(&openButton);
        openButton.setButtonText("Open..");
        openButton.addListener(this);
    
//~~~~~~GRANULATE TOGGLE
        addAndMakeVisible(&shouldGranulate);
        shouldGranulate.addListener(this);
        shouldGranulate.setButtonText("Granulate");
        //ENVELOPE COMBOBOX
        addAndMakeVisible(&envelopeMenu);
        envelopeMenu.setEnabled(false);
        envelopeMenu.addListener(this);
        envelopeMenu.setText("Envelope");
        envelopeMenu.addSectionHeading("Amplitude Envelope");
        envelopeMenu.addItem("Hann", 1);
        envelopeMenu.addItem("Hamming", 2);
        envelopeMenu.addItem("Blackman", 3);
        envelopeMenu.addItem("Blackman-Nuttall", 4);
        envelopeMenu.addItem("Triangle", 5);
        //GRANULATE LABELS
        addAndMakeVisible(&minGrain);
        addAndMakeVisible(&maxGrain);
        addAndMakeVisible(&minStretch);
        addAndMakeVisible(&maxStretch);
        addAndMakeVisible(&grainName);
        addAndMakeVisible(&newLength);
        newLength.setText("New file\n length", juce::NotificationType::dontSendNotification);
        grainName.setText("Grain (ms)", juce::NotificationType::dontSendNotification);
        addAndMakeVisible(&stretchName);
        stretchName.setText("Stretch", juce::NotificationType::dontSendNotification);
        addAndMakeVisible(&voiceName);
        voiceName.setText("Voices", juce::NotificationType::dontSendNotification);
        addAndMakeVisible(&numFilesName);
        numFilesName.setText("No. new files", juce::NotificationType::dontSendNotification);
        //VARI-GRAIN SLIDER
        addAndMakeVisible(&variGrainSlider);
        variGrainSlider.setEnabled(false);
        variGrainSlider.addListener(this);
        variGrainSlider.setSliderStyle(juce::Slider::SliderStyle::ThreeValueHorizontal);
        variGrainSlider.setColour(Slider::thumbColourId, Colours::limegreen);
        variGrainSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxRight, false, 45, 20);
        variGrainSlider.setRange(5, 2000);
        variGrainSlider.setMinAndMaxValues(50,50);
        variGrainSlider.setValue(50);
        variGrainSlider.setSkewFactor(0.4);
        //VARI-STRETCH SLIDER
        addAndMakeVisible(&variStretchSlider);
        variStretchSlider.setEnabled(false);
        variStretchSlider.addListener(this);
        variStretchSlider.setSliderStyle(juce::Slider::SliderStyle::ThreeValueHorizontal);
        variStretchSlider.setColour(Slider::thumbColourId, Colours::palevioletred);
        variStretchSlider.setColour(Slider::thumbColourId, Colours::mediumturquoise);
        variStretchSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxRight, false, 45, 20);
        variStretchSlider.setRange(0.1, 10.0, 0.1);
        variStretchSlider.setMinAndMaxValues(1.0, 1.0);
        variStretchSlider.setValue(1.0);
        variStretchSlider.setSkewFactor(0.7);
        variStretchSlider.setTextValueSuffix("x");
        //VOICES SLIDER
        addAndMakeVisible(&voicesSlider);
        voicesSlider.setEnabled(false);
        voicesSlider.addListener(this);
        voicesSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
        voicesSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxRight, false, 30, 25);
        voicesSlider.setRange(1, 30, 1);
        //NUMBER OF NEW FILES SLIDER
        addAndMakeVisible(&numOutFilesSlider);
        numOutFilesSlider.setEnabled(false);
        numOutFilesSlider.addListener(this);
        numOutFilesSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
        numOutFilesSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxRight, false, 30, 25);
        numOutFilesSlider.setRange(1, 10, 1);
        //NEW FILE LENGTH FIELD
        addAndMakeVisible(&newFileLength);
        newFileLength.setEnabled(false);
        newFileLength.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
        newFileLength.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxLeft, false, 80, 25);
        newFileLength.setRange(100, 600000, 100);
        newFileLength.setValue(1000);
        newFileLength.addListener(this);
        newFileLength.setTextValueSuffix("ms");

//~~~~~~RESAMPLE TOGGLE
        addAndMakeVisible(&shouldResample);
        shouldResample.addListener(this);
        shouldResample.setButtonText("Resample");
        //SAMPLE RATE COMBOBOX
        addAndMakeVisible(&sampleRateMenu);
        sampleRateMenu.setEnabled(false);
        sampleRateMenu.addListener(this);
        sampleRateMenu.setText("Sample Rate");
        sampleRateMenu.addSectionHeading("Sample Rate");
        sampleRateMenu.addItem("11,025Hz", 11025);
        sampleRateMenu.addItem("22,050Hz", 22050);
        sampleRateMenu.addItem("44,100Hz", 44100);
        sampleRateMenu.addItem("48,000Hz", 48000);
        sampleRateMenu.addItem("88,200Hz", 88200);
        sampleRateMenu.addItem("96,000Hz", 96000);
        sampleRateMenu.addItem("192,000Hz", 192000);
        //BIT RATE COMBOBOX
        addAndMakeVisible(&bitRateMenu);
        bitRateMenu.setEnabled(false);
        bitRateMenu.addListener(this);
        bitRateMenu.setText("Bit Rate");
        bitRateMenu.addSectionHeading("Bit Rate");
        
        bitRateMenu.addItem("16bit", 16);
        bitRateMenu.addItem("24bit", 24);
        bitRateMenu.addItem("32bit", 32);
        //STEREO TOGGLE
        addAndMakeVisible(&stereoToggle);
        stereoToggle.setEnabled(false);
        stereoToggle.addListener(this);
        stereoToggle.setButtonText("Stereo");
        stereoToggle.setToggleState(true, juce::NotificationType::sendNotification);
        //APPEND NAME COMBOBOX
        addAndMakeVisible(&appendNameMenu);
        appendNameMenu.setEnabled(false);
        appendNameMenu.addListener(this);
        appendNameMenu.setText("Append to name");
        appendNameMenu.addSectionHeading("Append to name");
        appendNameMenu.addItem("Keep original", 4);
        appendNameMenu.addItem("*_SampleRate.wav", 1);
        appendNameMenu.addItem("*_BitRate.wav", 2);
        appendNameMenu.addItem("*_SR_BR.wav", 3);
        //INTERPOLATION COMBOBOX
        addAndMakeVisible(&interpolationMenu);
        interpolationMenu.setEnabled(false);
        interpolationMenu.addListener(this);
        interpolationMenu.setText("Interpolation");
        interpolationMenu.addSectionHeading("Interpolation");
        interpolationMenu.addItem("Linear", 1); //item id cannot be 0
        interpolationMenu.addItem("Cosine", 2);
        interpolationMenu.addItem("Cubic", 3);
        interpolationMenu.addItem("Lagrange", 4); //native JUCE samplerate interpolation + filtering
        
        
//~~~~~~DELETE ORIGINALS TOGGLE
        addAndMakeVisible(&deleteOriginalsToggle);
        deleteOriginalsToggle.setEnabled(false);
        deleteOriginalsToggle.addListener(this);
        deleteOriginalsToggle.setButtonText("Delete original files");
        deleteOriginalsToggle.setToggleState(false, juce::NotificationType::dontSendNotification);

        //ITEM COUNT LABEL
        addAndMakeVisible(&queuedFileCount);
        queuedFileCount.setEnabled(false);
        queuedFileCount.setText("Browse or drag & drop files to process");
        
        //INSTRUCTION LABELS
        addAndMakeVisible(instructionOne);
        instructionOne.setText("Export to:", juce::NotificationType::dontSendNotification);
        //OUTPUT DIRECTORY LABEL
        addAndMakeVisible(&outputDirectory);
        outputDirectory.setEnabled(false);
        //BROWSE DIRECTORY BUTTON
        addAndMakeVisible(&browseDirectory);
        browseDirectory.setEnabled(false);
        browseDirectory.addListener(this);
        browseDirectory.setButtonText("Browse..");
        
        //CONVERT BUTTON
        addAndMakeVisible(&convertButton);
        convertButton.setButtonText("Convert");
        convertButton.setEnabled(false);
        convertButton.addListener(this);
        
        //OPEN CONTAINING FOLDER BUTTON
        addAndMakeVisible(&openContainingButton);
        openContainingButton.setEnabled(false);
        openContainingButton.addListener(this);
        openContainingButton.setButtonText("Open containing folder..");
    }
    
    void paint (Graphics& g) override
    {
        g.fillAll(Colours::palevioletred); //main window background colour
        g.drawRoundedRectangle(15, 125, getWidth()-30, 130, 10, 1); //rectangle around granulation settings
        g.drawRoundedRectangle(15, 280, getWidth()-30, 90, 10, 1); //rectangle around resampling settings
        g.drawRoundedRectangle(0, 0, getWidth(), getHeight(), 10, 10); //rectangle around main window
        g.drawText("SXRvert v0.1 by Dennis Lapchenko", getWidth()/2-100, 5, 200, 12, Justification::centred);
    }
    
    //sets/resets the position and size of all ui elements
    void resized() override
    {
        windowCloseButton.setBounds(4, 4, 15, 15);
        openButton.setBounds((getWidth()/2)-55, 25, 110, 70);
        
        //GRANULATE
        shouldGranulate.setBounds(15, 100, 100, 30);
        variGrainSlider.setBounds(15, 135, 310, 21);
            minGrain.setBounds(15, 130, 50, 10);
            maxGrain.setBounds(228, 151, 50, 12);
            grainName.setBounds(325, 140, 63, 10);
        variStretchSlider.setBounds(15, 160, 310, 20);
            minStretch.setBounds(15, 145, 50, 30);
            maxStretch.setBounds(228, 176, 50, 10);
            stretchName.setBounds(325, 165, 60, 10);
        
        voicesSlider.setBounds(30, 193, 90, 25);
        voiceName.setBounds(119, 193, 100, 30);
        numOutFilesSlider.setBounds(200, 193, 90, 25);
        numFilesName.setBounds(290, 193, 100, 30);
        envelopeMenu.setBounds(32, 225, 140, 25);
        newFileLength.setBounds(202, 225, 125, 24);
            newLength.setBounds(323, 222, 60, 30);
        
        //RESAMPLE
        shouldResample.setBounds(15, 255, 100, 30);
        sampleRateMenu.setBounds((getWidth()/11), 290, 140, 30);
        bitRateMenu.setBounds((getWidth()/10*5), 290, 100, 30);
        stereoToggle.setBounds((getWidth()/10*7.7f), 290, 100, 30);
        interpolationMenu.setBounds((getWidth()/11), 330, 140, 30);
        appendNameMenu.setBounds((getWidth()/10*5), 330, 140, 30);
        
        //GENERAL
        queuedFileCount.setBounds(10, 395, getWidth() - 20, 25);
        
        instructionOne.setBounds(5, 445, 100, 15);
        outputDirectory.setBounds(10, 460, getWidth() -70, 20);
        browseDirectory.setBounds(getWidth()-58, 460, 50, 20);
        deleteOriginalsToggle.setBounds(7, 480, 150, 30);
        
        convertButton.setBounds((getWidth()/2)-67.5, 535, 130, 80);
        openContainingButton.setBounds(7, getHeight()-27, getWidth()/3, 20);
        
    }
    
    //BUTTON CALLBACKS
    void buttonClicked(Button* button) override
    {
        if (button == &windowCloseButton) JUCEApplication::getInstance()->systemRequestedQuit();
        if (button == &openButton) openButtonClicked();
        
        if (button == &shouldResample) setResampleButtonsEnabled(button->getToggleState());
        if (button == &stereoToggle) stereoToggleClicked(button->getToggleState());
        
        if (button == &shouldGranulate) setGranulateButtonsEnabled(button->getToggleState());
        
        if (button == &convertButton) convertButtonClicked();
        if (button == &openContainingButton) openedFiles->getFirst().revealToUser();
        if (button == &browseDirectory) browseSaveDirectory();
    }
    
    //DROPDOWN MENU CALLBACKS
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override //listens for dropdown menu changes and applies them accordingly
    {
        if (comboBoxThatHasChanged == &sampleRateMenu)
            newSampleRate = comboBoxThatHasChanged->getItemId(comboBoxThatHasChanged->getSelectedItemIndex());
        if (comboBoxThatHasChanged == &bitRateMenu)
            newBitRate = comboBoxThatHasChanged->getItemId(comboBoxThatHasChanged->getSelectedItemIndex());
        if (comboBoxThatHasChanged == &interpolationMenu)
            interp = (Interpolation)comboBoxThatHasChanged->getItemId(comboBoxThatHasChanged->getSelectedItemIndex());
    }
    
    //SLIDER CALLBACKS
    void sliderValueChanged(Slider* changedSlider) override
    {
        if (changedSlider == &variGrainSlider)
        {
            minGrain.setText(String((int)changedSlider->getMinValue()), juce::NotificationType::dontSendNotification);
            maxGrain.setText(String((int)changedSlider->getMaxValue()), juce::NotificationType::dontSendNotification);
            changedSlider->setValue(floorf(changedSlider->getValue()));
        }
        if (changedSlider == &variStretchSlider)
        {
            minStretch.setText(String(changedSlider->getMinValue()), juce::NotificationType::dontSendNotification);
            maxStretch.setText(String(changedSlider->getMaxValue()), juce::NotificationType::dontSendNotification);
        }
    }
    
    //enable granulation settings buttons
    void setGranulateButtonsEnabled(bool state)
    {
        voicesSlider.setEnabled(state);
        variGrainSlider.setEnabled(state);
        variStretchSlider.setEnabled(state);
        numOutFilesSlider.setEnabled(state);
        envelopeMenu.setEnabled(state);
        newFileLength.setEnabled(state);
        
        if(shouldResample.getToggleState())
        {//if granulate is enabled, and resampling state is also on, disable resampling name append, since granulates is used
            if(state) appendNameMenu.setEnabled(false);
            else appendNameMenu.setEnabled(true);
        }
        
        if(state) envelopeMenu.setSelectedId(3); //set default envelope when granulate is selected
    }
    
    //enable resample settings buttons
    void setResampleButtonsEnabled(bool state)
    {
        sampleRateMenu.setEnabled(state);
        bitRateMenu.setEnabled(state);
        interpolationMenu.setEnabled(state);
        stereoToggle.setEnabled(state);
        appendNameMenu.setEnabled(state);
        
        if(shouldGranulate.getToggleState())
        {//if granulate is enabled, and resampling state is also on, disable resampling name append, since granulates is used
            if(state) appendNameMenu.setEnabled(false);
        }

        //set default values when resample is selected
        if(state)
        {
            sampleRateMenu.setSelectedId(96000);
            bitRateMenu.setSelectedId(24);
            interpolationMenu.setSelectedId(3);
            appendNameMenu.setSelectedId(1);
        }
    }
    
    //enable general buttons
    void setGeneralButtonsEnabled(bool state)
    {
        shouldGranulate.setToggleState(state, juce::NotificationType::sendNotification);
        deleteOriginalsToggle.setEnabled(state);
        openContainingButton.setEnabled(state);
        browseDirectory.setEnabled(state);
        convertButton.setEnabled(state);
        if(state) convertButton.setColour(TextButton::buttonColourId, Colours::palegreen);
        else convertButton.setColour(TextButton::buttonColourId, Colours::palevioletred);
    }
    
    //new window dragging callbacks, because after removing the title bar the standard dragging cappability doesn't work
    void mouseDown(const MouseEvent& e) override
    {
        windowDragger.startDraggingComponent(this, e);
    }
    void mouseDrag(const MouseEvent& e) override
    {
        windowDragger.dragComponent(this, e, nullptr);
    }
    
    void browseSaveDirectory()
    {
        if(chooser->browseForDirectory())
        {
            outputFolder = chooser->getResult().getFullPathName();
            outputDirectory.setText(outputFolder);
        }
    }
    
    enum Interpolation : Byte
    {
        LINEAR,
        COSINE,
        CUBIC,
        //HERMITE = 3,
        LAGRANGE
    };
//=====================================================================================================================
    
    
private:
        TextEditor queuedFileCount;
        TextEditor outputDirectory;
    
        TextButton windowCloseButton;
        TextButton openButton;
        TextButton browseDirectory;
        TextButton convertButton;
        TextButton openContainingButton;
        Label instructionOne;
    
        ComboBox sampleRateMenu;
        ComboBox bitRateMenu;
        ComboBox appendNameMenu;
        ComboBox interpolationMenu;
    
        ToggleButton stereoToggle;
        ToggleButton deleteOriginalsToggle;
        ToggleButton shouldGranulate;
        ToggleButton shouldResample;
    
        Slider variGrainSlider;
        Label minGrain, maxGrain;
        Slider variStretchSlider;
        Label minStretch, maxStretch;
        Slider voicesSlider;
        Slider numOutFilesSlider;
        Label grainName, stretchName, voiceName, numFilesName, newLength;
        ComboBox envelopeMenu;
        Slider newFileLength;
    
    
        AudioFormatManager formatManager;
        ScopedPointer<FileChooser> chooser = new FileChooser("Select a Wave file to convert...", File::nonexistent, "*.wav");
        Array<File> dragAndDroppedFiles;
        const Array<File>* openedFiles;
    
        int newSampleRate;
        int newBitRate;
        int numChannels;
    
        int granulatedFileCounter;
    
        Interpolation interp;
        String outputFolder;
    
        ComponentDragger windowDragger;
        bool writeConsoleProgress = true;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()     { return new MainContentComponent(); }

#endif  // MAINCOMPONENT_H_INCLUDED
