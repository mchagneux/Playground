# TODO 
*Delay the GUI side for now, concentrate on how to deal with audio and its intricacies*

- [x] Make a static layout, but make sure that everything is defined such that each block of the VST could be hosted in separated plugins. In particular each sub-component should hold its own state, which are added to the main state after. 
- [x] Write a component that embeds the Cmajor engine and can be used to hot reload components. Check whether it can be included in the chain dynamically. If not, use the audiograph.
    - [ ] Add the parameters in a cleaner way to the tree and the APVTS similarly to the rest of the chain.
    - [ ] Check that file changes work.
- [ ] Add something that involves a neural network (preferably using libtorch). Use/imitate things from TU Studio for this.



- [ ] Include Diopser by robert-vdh in Rust, built as a static lib. 
- [ ] Write a GUI and add a custom look & feel. Check OpenGL Then think whether I want to keep using JUCE GUI or not. Write a waveform visualizer. Write a filter magnitude response.





