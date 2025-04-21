# Coding

### Pointers

Anytime you're storing a pointer, it should be a unique_ptr or shared_ptr

Anytime you're passing a pointer into a function for reading/writing but not storing it anywhere, it should be a raw pointer or a reference instead

DO NOT accept a pointer as a method parameter and copy it into an instance variable, as that changes the pointer's lifetime dynamics and may confuse the caller.