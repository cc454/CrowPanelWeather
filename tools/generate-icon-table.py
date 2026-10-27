from pathlib import Path
import json
root=Path(__file__).resolve().parents[1]
m=json.loads((root/'assets/manifest.json').read_text())
names=sorted(key[:-8] for key in m['assets'] if key.endswith('/current'))
lines=['#pragma once','#include <cstring>','#include "assets/include/icons_current.h"','#include "assets/include/icons_24.h"',
       'struct IconAsset { const char* name; const uint8_t* current; int currentWidth; const uint8_t* forecast; };',
       'static const IconAsset kIcons[] = {']
for name in names:
    var=name.replace('-','_')
    lines.append(f'  {{"{name}", cpw_assets::{var}_current, cpw_assets::{var}_current_width, cpw_assets::{var}_24}},')
lines+=['};','inline const IconAsset* findIcon(const char* name) {',
        '  for (const auto& icon : kIcons) if (strcmp(name,icon.name)==0) return &icon;',
        '  for (const auto& icon : kIcons) if (strcmp("unknown",icon.name)==0) return &icon;',
        '  return &kIcons[0];','}','']
(root/'include/icon_table.h').write_text('\n'.join(lines))
