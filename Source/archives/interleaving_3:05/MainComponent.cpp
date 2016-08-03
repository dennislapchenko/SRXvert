#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class MainContentComponent   :  public Component,
                                public FileDragAndDropTarget,
                                public Button::Listener,
                                public ComboBox::Listener
{
public:
    MainContentComponent()
    {
        setSize (400, 500);
        initializeGUI();
        formatManager.registerBasicFormats();
        
        automatedTesting();
    }

    ~MainContentComponent()
    {
        shutdownComponent();
    }
    
    void automatedTesting()
    {
        File newFile("/Users/DennisL/Development/JUCE/SRXvert/TestData/Glass Container Break 2.wav");
        dragAndDroppedFiles.add(newFile);
        openedFiles = &dragAndDroppedFiles;
        openButtonClicked(false);
        sampleRateMenu.setSelectedId(96000);
        bitRateMenu.setSelectedId(24);
        appendNameMenu.setSelectedId(1);
    }
    
    void openButtonClicked(bool shouldChooseFiles)
    {
        if(shouldChooseFiles)
        {
            if(chooser->browseForMultipleFilesToOpen()) //choose files
            {
                openedFiles = &chooser->getResults(); //save chosen result address in a pointer
            }
        }
        queuedFileCount.setText(String(openedFiles->size())+ " " +(openedFiles->size() == 1 ? "file" : "files") + " queued for converting"); //update GUI
        sampleRateMenu.setEnabled(true); //enable all setting options
        bitRateMenu.setEnabled(true);
        stereoToggle.setEnabled(true);
        deleteOriginalsToggle.setEnabled(true);
        appendNameMenu.setEnabled(true);
        openContainingButton.setEnabled(true);
    }
    
    void convertButtonClicked()
    {
        ScopedPointer<WavAudioFormat> wav = new WavAudioFormat();
        
        for (const File outFile : *openedFiles)
        {
            ScopedPointer<AudioFormatReader> reader = formatManager.createReaderFor(outFile);
            String outputPath = buildOutputPath(outFile.getFullPathName()); //get original path and append settings to the name
            File outputFile = File(outputPath); //create new file to be written
            outputFile.deleteFile(); //safecheck to delete the existing file before writing new one
            
            ScopedPointer<FileOutputStream> outStream (outputFile.createOutputStream());
            ScopedPointer<AudioFormatWriter> writer (wav->createWriterFor(outStream, newSampleRate, numChannels, newBitRate, reader->metadataValues, 0));
            
            if(reader != nullptr)
            {
                ScopedPointer<AudioFormatReaderSource> readerSource = new AudioFormatReaderSource(reader, true); //Source that shoves read file into a big buffer
                const StringArray metaKeys = reader->metadataValues.getAllKeys();
                const StringArray metaValues = reader->metadataValues.getAllValues();
                
                for(int i = 0; i < metaKeys.size(); i++)
                {
                    DBG("Key: "+metaKeys[i]+" - Value: "+metaValues[i]);
                }
                
                AudioSampleBuffer rightChannel, leftChannel, interleavedChannel;
                
                leftChannel.setSize(1, reader->lengthInSamples);
                reader->read(&leftChannel, 0, reader->lengthInSamples, 0, true, false);
                
                rightChannel.setSize(1, reader->lengthInSamples);
                reader->read(&rightChannel, 0, reader->lengthInSamples, 0, false, true);
                
                interleavedChannel.setSize(1, reader->lengthInSamples*2);
                
                int writingIndex = 0;
                const float* leftSamples = leftChannel.getReadPointer(0); //get the pointer to the start of left channel samples array
                const float* rightSamples = rightChannel.getReadPointer(0); //get the pointer to the start of right channel samples array
                
                for(int monoIndex = 0; monoIndex < reader->lengthInSamples; monoIndex++)
                {
                    interleavedChannel.addSample(0, writingIndex++, leftSamples[monoIndex]);
                    interleavedChannel.addSample(0, writingIndex++, rightSamples[monoIndex]);
                }
                
                
                AudioTransportSource resamplingSource; //Source which converts SR under the hood
                resamplingSource.setSource(readerSource, 0, nullptr, reader->sampleRate);
                resamplingSource.prepareToPlay(2048, newSampleRate);
                resamplingSource.start();
                
                //writer->writeFromAudioSource(resamplingSource, resamplingSource.getTotalLength()); //writes the source containing the buffers int oa file
                
                writer->writeFromAudioSampleBuffer(interleavedChannel, 0, interleavedChannel.getNumSamples());
                
                leftChannel.clear();
                rightChannel.clear();
            }
            outStream.release(); //reset output stream for next file
            reader.release(); //reset file stream reader for next file
            
            if(deleteOriginalsToggle.getToggleState() && appendNameMenu.getSelectedId() != 4) //deletes the original files if selected by the user
            {
                outFile.deleteFile();
            }
            
            outputFile.revealToUser(); //open containing folder with converted files
        }
    };
    
    AudioBuffer<float> interleaveChannels()
    {
        AudioBuffer<float> result;
        
        return result;
    }
    
    void stereoToggleClicked(bool toggled)
    {
        if(toggled) numChannels = 2;
        else        numChannels = 1;
    }
    
    String buildOutputPath(String originalPath)
    {
        String result = originalPath.trimCharactersAtEnd(".wav");
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
    
    bool isInterestedInFileDrag(const StringArray& files) override
    {
        openButton.setColour(TextButton::buttonColourId, Colours::green);
        return true;
    }
    
    void fileDragExit (const StringArray& files) override
    {
        openButton.setColour(TextButton::buttonColourId, Colours::white);
    }
    
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
            openButtonClicked(false);
        }
    }
    
    
    void shutdownComponent()
    {
    }

//===GUI===============================================================================================================
    void initializeGUI()
    {
        //OPEN BUTTON
        addAndMakeVisible(&openButton);
        openButton.setButtonText("Open..");
        openButton.addListener(this);
        
        //ITEM COUNT LABEL
        addAndMakeVisible(&queuedFileCount);
        queuedFileCount.setEnabled(true);
        
        //SAMPLE RATE COMBOBOX
        addAndMakeVisible(&sampleRateMenu);
        sampleRateMenu.setEnabled(false);
        sampleRateMenu.addListener(this);
        sampleRateMenu.setText("Sample Rate");
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
        bitRateMenu.addItem("16bit", 16);
        bitRateMenu.addItem("24bit", 24);
        bitRateMenu.addItem("32bit", 32);
        
        //STEREO TOGGLE
        addAndMakeVisible(&stereoToggle);
        stereoToggle.setEnabled(false);
        stereoToggle.addListener(this);
        stereoToggle.setButtonText("Stereo");
        stereoToggle.setToggleState(true, juce::NotificationType::sendNotification);
        
        //DELETE ORIGINALS TOGGLE
        addAndMakeVisible(&deleteOriginalsToggle);
        deleteOriginalsToggle.setEnabled(false);
        deleteOriginalsToggle.addListener(this);
        deleteOriginalsToggle.setButtonText("Delete original files");
        deleteOriginalsToggle.setToggleState(false, juce::NotificationType::dontSendNotification);
        
        //APPEND NAME COMBOBOX
        addAndMakeVisible(&appendNameMenu);
        appendNameMenu.setEnabled(false);
        appendNameMenu.addListener(this);
        appendNameMenu.setText("Append to name");
        appendNameMenu.addItem("Keep original", 4);
        appendNameMenu.addItem("*_SampleRate.wav", 1);
        appendNameMenu.addItem("*_BitRate.wav", 2);
        appendNameMenu.addItem("*_SR_BR.wav", 3);

        //INSTRUCTION LABELS
        addAndMakeVisible(instructionOne);
        instructionOne.setText("Files will be saved in source directory, appending rates if chosen", juce::NotificationType::dontSendNotification);
        addAndMakeVisible(instructionTwo);
        instructionTwo.setText("..or drop!", juce::NotificationType::dontSendNotification);
        
        //OPEN CONTAINING FOLDER BUTTON
        addAndMakeVisible(&openContainingButton);
        openContainingButton.setEnabled(false);
        openContainingButton.addListener(this);
        openContainingButton.setButtonText("Open containing folder..");
        
        //CONVERT BUTTON
        addAndMakeVisible(&convertButton);
        convertButton.setButtonText("Convert");
        convertButton.setEnabled(false);
        convertButton.addListener(this);
        
        //WINDOW CLOSE BUTTON
        addAndMakeVisible(&windowCloseButton);
        windowCloseButton.setButtonText("X");
        windowCloseButton.setEnabled(true);
        windowCloseButton.addListener(this);
        windowCloseButton.setColour(TextButton::buttonColourId, Colours::indianred);
    }
    
    void paint (Graphics& g) override
    {
        g.fillAll(Colours::bisque); //main window background colour
        g.drawRoundedRectangle(10, 115, getWidth()-20, 85, 5, 1); //rectangle around settings options
        g.drawRoundedRectangle(0, 0, 400, 500, 10, 8); //rectangle around main window
    }
    
    void resized() override //sets/resets the position and size of all ui elements
    {
        windowCloseButton.setBounds(5, 5, 15, 15);
        openButton.setBounds((getWidth()/2)-(getWidth()/5/2), 20, getWidth()/5, getHeight()/6);
        instructionOne.setBounds(5, getHeight()/4*2-30, getWidth() - 10, 20);
        instructionTwo.setBounds((getWidth()/3*2), 20, 100, 20);
        queuedFileCount.setBounds(10, getHeight()/4*2, getWidth() - 20, 20);
        openContainingButton.setBounds(5,getHeight()-25, getWidth()/3, 20);
        convertButton.setBounds((getWidth()/2)-(getWidth()/3/2), getHeight()/4*3, getWidth()/3, getHeight()/6);
        
        sampleRateMenu.setBounds((getWidth()/11), getHeight()/4, 120, 30);
        bitRateMenu.setBounds((getWidth()/10*5), getHeight()/4, 100, 30);
        stereoToggle.setBounds((getWidth()/10*8), getHeight()/4, 100, 30);
        deleteOriginalsToggle.setBounds((getWidth()/12), getHeight()/3, 150, 30);
        appendNameMenu.setBounds((getWidth()/10*5), getHeight()/3, 140, 25);
    }
    
    void buttonClicked(Button* button) override //button callbacks
    {
        if (button == &openButton) openButtonClicked(true);
        if (button == &convertButton) convertButtonClicked();
        if (button == &stereoToggle) stereoToggleClicked(button->getToggleState());
        if (button == &openContainingButton) openedFiles->getFirst().revealToUser();
        if (button == &windowCloseButton) JUCEApplication::getInstance()->systemRequestedQuit();
    }
    
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override //listens for dropdown menu changes and applies them accordingly
    {
        if (comboBoxThatHasChanged == &sampleRateMenu) newSampleRate = comboBoxThatHasChanged->getItemId(comboBoxThatHasChanged->getSelectedItemIndex());
        if (comboBoxThatHasChanged == &bitRateMenu) newBitRate = comboBoxThatHasChanged->getItemId(comboBoxThatHasChanged->getSelectedItemIndex());
        
        if(sampleRateMenu.getSelectedItemIndex() != -1 && bitRateMenu.getSelectedItemIndex() != -1 && appendNameMenu.getSelectedItemIndex() != -1)
        {
           convertButton.setEnabled(true); //enables CONVERT button only if SR, bitrate and appendName is selected
        }
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        windowDragger.startDraggingComponent(this, e);
    }
    
    void mouseDrag(const MouseEvent& e) override
    {
        windowDragger.dragComponent(this, e, nullptr);
    }
//=====================================================================================================================
    
    
private:
        TextButton windowCloseButton;
        TextButton openButton;
        TextEditor queuedFileCount;
        TextButton convertButton;
        TextButton openContainingButton;
        Label instructionOne;
        Label instructionTwo;
        ComboBox sampleRateMenu;
        ComboBox bitRateMenu;
        ComboBox appendNameMenu;
        ToggleButton stereoToggle;
        ToggleButton deleteOriginalsToggle;
    
    
        AudioFormatManager formatManager;
        ScopedPointer<FileChooser> chooser = new FileChooser("Select a Wave file to convert...", File::nonexistent, "*.wav");
        Array<File> dragAndDroppedFiles;
        const Array<File>* openedFiles;
    
        int newSampleRate;
        int newBitRate;
        int numChannels;
    
        ComponentDragger windowDragger;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()     { return new MainContentComponent(); }

#endif  // MAINCOMPONENT_H_INCLUDED
