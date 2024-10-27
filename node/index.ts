/**
 * WIP MVP for importing a Native Node Binding to call engine functions
 */

// @ts-ignore
import { createWindow, windowShouldClose, windowUpdate, windowPressedEsc, closeWindow } from '../artifacts/node-vk-app/Release/node';

createWindow();

while (!windowShouldClose()) {
    windowUpdate();
    if (windowPressedEsc()) {
        closeWindow();
    }
}