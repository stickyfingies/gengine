"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
// @ts-ignore
var node_node_1 = require("../artifacts/node-vk-app/Release/node.node");
(0, node_node_1.createWindow)();
while (!(0, node_node_1.windowShouldClose)()) {
    (0, node_node_1.windowUpdate)();
    if ((0, node_node_1.windowPressedEsc)()) {
        (0, node_node_1.closeWindow)();
    }
}
