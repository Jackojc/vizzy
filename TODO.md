# TODO

### MIDI
- LibreMIDI setup and boilerplate
- Add argument to specify MIDI device/port to read from
- Setup MIDI callback to communicate with main thread safely
- Create envelopes structure to track envelope progress and state
  - Multistage envelopes (`vector<pair<envelopestage, int>>`)
- Track active envelopes in vector/map
- MIDI events insert/remove envelopes as needed

### Lua
- Hook into Lua and setup global state and callback functions
  - `on_load`, `on_update`, `on_event`
- Setup shader loading from Lua (we might only need to setup fragment shaders
  since we don't care about meshes)
- Setup event handling from Lua
- Setup data to be sent to shader `on_update` (SSBO)

### OpenGL
- Move to using compute shaders so we can have the ability to mix & match shaders
  like pre-made effects etc.
- Rendering to user defined framebuffer and binding multiple shaders in sequence
- Allow for visual feedback like effects

# Considerations
- Envelopes normalised to values between 0-1
- How to track envelope mapped to specific device OR note
  - List of events (specific/global notes, CC) to track for an envelope?

# Future work
- SPIR-V shaders
- SPIR-V disassembler
- WGSL
