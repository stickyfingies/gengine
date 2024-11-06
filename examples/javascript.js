/**
 * This file uses JS bindings from javascript.cpp to create a game scene.
 */

print("Hello, JS!");

/**
 * This function is called after javascript.cpp starts the core engine systems.
 * @param {*} scene a wrapper around the C++ SceneBuilder API
 * @todo change laodModel() to accept arrays of data
 */
function create(scene) {

    const models = [
        { path: "./data/spinny.obj", flip_uvs: false, flip_tris: true },
        { path: "./data/map.obj", flip_uvs: true, flip_tris: true }
    ];

    for (var i = 0; i < models.length; i++) {
        loadModel(scene, models[i].path, new VisualModelSettings(models[i].flip_uvs, models[i].flip_tris, false));
    }

    // const player = { location: [20, 100, 20], shape: CAPSULE, mass: 70, vanity: "./data/spinny.obj" };

    // const player = new Entity()
    //     .has(new Location(20, 100, 20))
    //     .has(new SolidCapsule({ mass: 70 }))
    //     .has(new Model3D("./data/spinny.obj"));

    // player
    const playerTransform = new Transform().translate(20, 100, 20);
    createCapsule(scene, playerTransform, /* mass */ 70.0, "./data/spinny.obj");

    // ball
    const ballTransform = new Transform().translate(10, 100, 0).scale(6, 6, 6);
    createSphere(scene, ballTransform, /* mass */ 62, /* radius */ 1, "./data/spinny.obj");
}