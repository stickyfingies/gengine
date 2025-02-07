"use strict";
/**
 * WIP MVP for importing a Native Node Binding to call engine functions
 */
Object.defineProperty(exports, "__esModule", { value: true });
// @ts-ignore
var node_1 = require("../artifacts/node-vk-app/Release/node");
(0, node_1.createWindow)();
while (!(0, node_1.windowShouldClose)()) {
    (0, node_1.windowUpdate)();
    if ((0, node_1.windowPressedEsc)()) {
        (0, node_1.closeWindow)();
    }
}
