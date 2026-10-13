# Traducciones

El idioma fuente es **inglés** (las cadenas se escriben en inglés dentro de
`qsTr()`/`tr()`). Las traducciones se mantienen como ficheros **gettext `.po`** en
este directorio (`luma_<lang>.po`), un formato cómodo para Flathub/Weblate.

## Flujo de trabajo

1. Instala las herramientas de Qt Linguist:

   ```
   sudo dnf install qt6-linguist
   ```

2. Extrae las cadenas y regenera/actualiza los `.po`:

   ```
   translations/update-translations.sh
   ```

3. Edita cada `translations/luma_<lang>.po` rellenando los `msgstr`.

4. Reconstruye. CMake (si encuentra las Linguist tools) compila cada `.po` a un
   `.qm` embebido en `:/i18n`. La app carga el `.qm` según el idioma elegido en
   **Preferencias → Language** o, si es «System», el locale del sistema.

## Idiomas incluidos

Se traducen solo los idiomas cuya calidad se puede asegurar; el resto caen a
inglés hasta que se añadan sus `.po`. La lista de idiomas compilados está en
`LUMA_LANGUAGES` (CMakeLists.txt) y la lista mostrada en el selector, en
`AppSettings::availableLanguages()`.
