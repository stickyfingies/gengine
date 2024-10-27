/**
 * This file uses the C++ API for NodeJS native bindings.
 * 
 * It wraps some basic engine functions, and exposes them to Node.
 * Then, you can import the package from node, and call the functions.
 */

#include <GLFW/glfw3.h>
#include <iostream>
#include <napi.h>

GLFWwindow* window;

static void CreateWindow(const Napi::CallbackInfo& info)
{
    glfwInit();
	window = glfwCreateWindow(1280, 720, "Gengine", nullptr, nullptr);
}

Napi::Value WindowShouldClose(const Napi::CallbackInfo& info)
{
	return Napi::Boolean::New(info.Env(), glfwWindowShouldClose(window));
}

void WindowUpdate(const Napi::CallbackInfo& info) {
    glfwPollEvents();
    glfwSwapBuffers(window);
}

Napi::Value WindowPressedEsc(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), glfwGetKey(window, GLFW_KEY_ESCAPE));
}

void CloseWindow(const Napi::CallbackInfo& info) {
    glfwSetWindowShouldClose(window, true);
}

static Napi::Object Init(Napi::Env env, Napi::Object exports)
{
	exports.Set(Napi::String::New(env, "createWindow"), Napi::Function::New(env, CreateWindow));
    exports.Set(Napi::String::New(env, "windowShouldClose"), Napi::Function::New(env, WindowShouldClose));
    exports.Set(Napi::String::New(env, "windowUpdate"), Napi::Function::New(env, WindowUpdate));
    exports.Set(Napi::String::New(env, "windowPressedEsc"), Napi::Function::New(env, WindowPressedEsc));
    exports.Set(Napi::String::New(env, "closeWindow"), Napi::Function::New(env, CloseWindow));
	return exports;
}

NODE_API_MODULE(hello, Init)