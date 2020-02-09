# public-examples
Examples of my C++ work for public review. These folders contain code snippets that I feel are of particular pride or interest from some of the projects mentioned in my portfolio that I am currently unable to make fully public. 

* Advanced Programming for Games: As part of a solution to the computational problem of, for any given 15-tile puzzle configuration, counting the number of continuous rows and columns, including reverse and partial rows/columns.
    * **15PuzzleSim.cpp:** My main simulation. Contains **a thread-safe buffer** to first write output to rather than slow down with repeated file opening and closing, **use of thread pools** to solve multiple puzzles simultaneously, **computational solution functions** for the simulation
    * **FileHandler.h:** My generic file handler featuring **templates** to account for the multiple values to be written to a solution file and **error checking.**
    * **UnitTests.cpp:** An example Visual Studio unit test suite used through development. 
* Advanced Graphics for Games: A selection of personal work in the task to render a scene that extended the OpenGL tutorials given to us.
    * ** **Particle.h** and **ParticleSystem.cpp**: the main components of the particle system used for the snow, demonstrating **particle life and reuse** based on the first unused.
    * **SceneNode.h**: my extension of a basic scene graph node to allow components of the scene to hold their own information such as transforms and shader data to allow the Renderer to do a cleaner job without needing to know which shaders etc to swap in and out. 
    * **WaterSceneNode.h**: an example inheritance of SceneNode.h to allow extra data to be specified for components that need it.
    * **Renderer.cpp**: a selection of functions from the main Renderer showing off the particle system and scene graph setup and use. 
* Advanced Game Technologies: 
