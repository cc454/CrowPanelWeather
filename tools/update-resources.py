"""Refresh verified Open-Meteo trust anchor and upstream IANA/POSIX timezone map."""
import json, ssl, socket, urllib.request
from pathlib import Path

root=Path(__file__).resolve().parents[1]
context=ssl.create_default_context()
with socket.create_connection(('api.open-meteo.com',443),timeout=20) as sock:
    with context.wrap_socket(sock,server_hostname='api.open-meteo.com') as tls:
        chain=tls._sslobj.get_verified_chain()
        pem=chain[-1].public_bytes()
        if isinstance(pem,bytes):pem=pem.decode()
        peer=tls.getpeercert()
(root/'include/weather_ca.h').write_text('#pragma once\n// Root from a verified TLS chain for api.open-meteo.com; refresh with tools/update-resources.py.\nstatic const char kWeatherCA[] = R"CA('+pem+')CA";\n')
url='https://raw.githubusercontent.com/nayarsystems/posix_tz_db/master/zones.json'
with urllib.request.urlopen(url,context=context,timeout=30) as response: zones=json.load(response)
lines=['#pragma once','// Generated from nayarsystems/posix_tz_db (MIT); see THIRD_PARTY.md.',
       'struct TimezoneEntry { const char* name; const char* posix; };','static const TimezoneEntry kTimezones[] = {']
lines += ['  {'+json.dumps(name)+', '+json.dumps(posix)+'},' for name,posix in sorted(zones.items())]
lines += ['};','']
(root/'include/timezones.h').write_text('\n'.join(lines))
print(json.dumps({'timezone_entries':len(zones),'verified_peer_issuer':peer['issuer']}))
