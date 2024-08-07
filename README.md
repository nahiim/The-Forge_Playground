# The-Forge_Playground

Welcome to The-Forge Playground! This project is a playground for exploring and experimenting with [The-Forge](https://github.com/ConfettiFX/The-Forge), a cross-platform rendering framework.

## Getting Started
### Clone The Repository

```bash
git clone https://github.com/nahiim/The-Forge_Playground.git
cd The-Forge_Playground
```

### Samples
In the samples directory you will find a few pre-written samples including a compute sample.
Create a new sample to play with by running `new_sample.bat`.

###
Compile all shaders of a particular sample by running `compile_shaders.bat` in the solution directory. The shaders are in the `shaders` of the solution directory.
It uses GLSL and the file extensions must be `.vert`, `.frag` and `.comp` for vertex, fragment and compute shaders respectively otherwise the script won't compile it.
Your shaders should also be in the `shaders` of the solution directory otherwise the script won't compile it.
If you want to write HLSL or FSL shaders you can do that and compile them in your own way. I made this because I find it convenient for myself.


## Note
This is only a learning excercise which I made for myself to easily experiment with [The-Forge](https://github.com/ConfettiFX/The-Forge). I made it public on the off chance that it might be useful to someone else learnning CGI programming.
