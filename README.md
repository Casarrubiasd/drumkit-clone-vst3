# DrumKit Clone (VST3)

Drum machine tipo TR-808 para Ableton Live 12 en macOS (universal: Apple Silicon + Intel).
8 voces sintetizadas (sin samples), secuenciador de 16 pasos sincronizado al tempo del host.

## Compilar en GitHub (sin Mac local)
1. Sube este repositorio a tu cuenta. El workflow `Build VST3 (macOS)` se ejecuta solo en cada push.
2. Ve a la pestaña **Actions**, abre la última ejecución y descarga el artefacto `DrumKitClone-VST3`.

## Compilar en tu Mac (opcional)
```bash
brew install cmake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
El bundle queda en `build/DrumkitClone_artefacts/Release/VST3/DrumKit Clone.vst3`.

## Instalar
```bash
mkdir -p ~/Library/Audio/Plug-Ins/VST3
cp -R "DrumKit Clone.vst3" ~/Library/Audio/Plug-Ins/VST3/
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"DrumKit Clone.vst3"
```
Si descargaste el zip, el último comando evita el bloqueo de Gatekeeper. Luego en Live: Preferencias > Plug-Ins > Rescan.

## Uso en Ableton Live 12
- Crea una pista MIDI, carga **DrumKit Clone** como instrumento.
- Con el transporte en Play, el secuenciador corre solo.
- Notas MIDI: C1 (36) bombo, C#1 (37) caja, D1 (38) hi-hat cerrado, D#1 (39) hi-hat abierto, E1 (40) palmas, F1 (41) rim, F#1 (42) tom grave, G1 (43) cowbell.
- Elige una voz con el botón de color navy para editar sus pasos en la fila inferior. Cada paso se enciende/apaga con un clic.
- Knobs rojos: nivel. Grises: afinación (±12 semitonos) y decay.

## Notas
- Requiere JUCE 8.0.4 (se descarga automáticamente con CMake FetchContent).
