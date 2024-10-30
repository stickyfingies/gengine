print("Hello, JS!");

function create(scene) {
    print("Creating scene...");

    loadModel(scene, "./data/spinny.obj", new VisualModelSettings(false, true, false));
    loadModel(scene, "./data/map.obj", new VisualModelSettings(true, true, false));

    // player
    const playerTransform = new Transform().translate(20, 100, 20);
    createCapsule(scene, playerTransform, /* mass */ 70.0, "./data/spinny.obj");

    // ball
    const ballTransform = new Transform().translate(10, 100, 0).scale(6, 6, 6);
    createSphere(scene, ballTransform, /* mass */ 62, /* radius */ 1, "./data/spinny.obj");
}