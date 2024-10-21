
// @ts-ignore
import { createWindow, windowShouldClose, windowUpdate, windowPressedEsc, closeWindow } from '../artifacts/node-vk-app/Release/node.node';

createWindow();

while (!windowShouldClose()) {
    windowUpdate();
    if (windowPressedEsc()) {
        closeWindow();
    }
}