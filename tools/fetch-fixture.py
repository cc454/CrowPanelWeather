"""Download an illustrative Zurich response for an opt-in live integration check."""
import json, ssl, urllib.request
from pathlib import Path
url=('https://api.open-meteo.com/v1/forecast?latitude=47.3769&longitude=8.5417'
     '&current=temperature_2m,apparent_temperature,weather_code,is_day,wind_speed_10m,wind_direction_10m'
     '&hourly=temperature_2m&daily=temperature_2m_max,temperature_2m_min,weather_code,precipitation_probability_max,uv_index_max,sunrise,sunset'
     '&past_days=1&forecast_days=7&timezone=auto&timeformat=unixtime&temperature_unit=celsius&wind_speed_unit=kmh')
root=Path(__file__).resolve().parents[1]
with urllib.request.urlopen(url,context=ssl.create_default_context(),timeout=30) as response:
    data=response.read(96*1024+1)
    assert len(data)<=96*1024
    weather=json.loads(data)
out=root/'tests/fixtures/open-meteo.json'
out.parent.mkdir(parents=True,exist_ok=True)
out.write_bytes(data)
print(json.dumps({'bytes':len(data),'timezone':weather['timezone'],'hours':len(weather['hourly']['time']),'days':len(weather['daily']['time'])}))
