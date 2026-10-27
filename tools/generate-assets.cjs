// Original CrowPanelWeather vector artwork. No network or third-party artwork.
const fs=require('fs'),path=require('path');
const {chromium}=require(process.env.CPW_NODE_MODULES?path.join(process.env.CPW_NODE_MODULES,'playwright'):'playwright');
const out=path.resolve(__dirname,'../assets');
const p=d=>`<path d="${d}"/>`,c=(x,y,r)=>`<circle cx="${x}" cy="${y}" r="${r}"/>`;
const sun=(x=24,y=22,r=10)=>c(x,y,r)+Array.from({length:8},(_,i)=>{const a=i*Math.PI/4;return p(`M${x+Math.cos(a)*(r+5)} ${y+Math.sin(a)*(r+5)}L${x+Math.cos(a)*(r+10)} ${y+Math.sin(a)*(r+10)}`)}).join('');
const moon=p('M40 7C23 5 12 16 12 31C12 47 25 57 40 54C49 52 55 46 58 38C42 43 27 28 40 7Z');
const cloud=p('M17 44C3 44 3 25 17 25C19 10 41 9 46 27C61 25 64 44 49 44Z');
const smallCloud=`<g transform="translate(8 12) scale(.8)">${cloud}</g>`;
const rain=(n=3)=>[20,32,44].slice(0,n).map(x=>p(`M${x+3} 50L${x-1} 58`)).join('');
const dots=[20,32,44].map(x=>c(x,54,1.5)).join('');
const flakes=(n=2)=>[21,43].slice(0,n).map(x=>p(`M${x} 49V61M${x-5} 52L${x+5} 58M${x-5} 58L${x+5} 52`)).join('');
const bolt=p('M34 46L25 54H34L29 62L45 49H36L41 46Z');
const icy=p('M17 57H25M21 53V61');
const mixed=night=>`<g transform="translate(1 -1) scale(.68)">${night?moon:sun()}</g>${smallCloud}`;
const icons={
 'clear-day':sun(32,32,13),'clear-night':moon,
 'mainly-clear-day':sun(27,25,11)+`<g transform="translate(24 31) scale(.52)">${cloud}</g>`,
 'mainly-clear-night':`<g transform="translate(-2 -1) scale(.8)">${moon}</g><g transform="translate(24 31) scale(.52)">${cloud}</g>`,
 'partly-cloudy-day':mixed(false),'partly-cloudy-night':mixed(true),overcast:cloud,
 fog:`${cloud}${p('M8 51H51M15 59H57')}`,
 'rime-fog':cloud+p('M7 51H57')+flakes(1),
 drizzle:cloud+dots,'freezing-drizzle':cloud+c(37,54,1.5)+c(49,54,1.5)+icy,
 rain:cloud+rain(),'heavy-rain':cloud+rain()+p('M12 49L8 57M54 49L50 57'),
 'freezing-rain':cloud+p('M38 50L34 58M51 50L47 58')+icy,
 snow:cloud+flakes(),'heavy-snow':cloud+flakes()+c(32,54,1.4),
 'snow-grains':cloud+[17,32,47].map(x=>`<rect x="${x-1.5}" y="53" width="3" height="3"/>`).join(''),
 'rain-showers-day':mixed(false)+rain(),'rain-showers-night':mixed(true)+rain(),
 'heavy-rain-showers':mixed(false)+rain()+p('M54 50L50 58'),
 'snow-showers':mixed(false)+flakes(),
 thunderstorm:cloud+bolt,'heavy-thunderstorm':cloud+bolt+p('M16 48L12 57M53 48L49 57'),
 'hail-thunderstorm':cloud+bolt+c(15,55,2.2)+c(52,55,2.2),
 unknown:c(32,32,24)+p('M24 23C24 12 43 12 43 24C43 31 32 30 32 39')+c(32,47,1),
 'ui-sunrise':p('M5 45H59M10 55H54M18 45A14 14 0 0 1 46 45M32 30V8M25 16L32 8L39 16M9 27L14 32M50 32L55 27'),
 'ui-sunset':p('M5 45H59M10 55H54M18 45A14 14 0 0 1 46 45M32 8V30M25 22L32 30L39 22M9 27L14 32M50 32L55 27'),
 'ui-wind':p('M6 24H44C57 24 57 7 45 7C40 7 37 10 37 14M6 34H52C63 34 63 49 52 49M6 44H27C38 44 38 59 27 59'),
 'ui-uv':sun(32,28,12)+p('M21 57H43'),
 'ui-temperature':p('M25 12A7 7 0 0 1 39 12V39A13 13 0 1 1 25 39ZM32 21V47M44 18H52M44 28H50')+c(32,48,4),
 'ui-precipitation':p('M32 6C26 17 13 31 13 42A19 19 0 0 0 51 42C51 31 38 17 32 6ZM22 40C20 47 26 52 31 52'),
 'ui-refresh':p('M53 24A23 23 0 0 0 12 18M12 6V18H24M11 40A23 23 0 0 0 52 46M52 58V46H40'),
 'ui-menu':p('M10 16H54M10 32H54M10 48H54'),
 'ui-back':p('M31 12L11 32L31 52M12 32H55'),
 'ui-up':p('M12 42L32 22L52 42'),'ui-down':p('M12 22L32 42L52 22'),
 'ui-confirm':p('M9 32L25 48L55 16'),
 'ui-wifi':p('M6 20Q32 0 58 20M15 31Q32 17 49 31M24 42Q32 35 40 42')+c(32,53,2),
 'ui-offline':p('M6 20Q32 0 58 20M15 31Q32 17 49 31M24 42Q32 35 40 42M7 7L57 57')+c(32,53,2),
 'ui-clock':c(32,32,25)+p('M32 15V32L44 39'),
 'ui-warning':p('M32 6L60 56H4ZM32 23V38')+c(32,47,1.5),
 'ui-battery':p('M5 17H53V47H5ZM53 27H59V37H53M12 24H44V40H12Z'),
 'ui-battery-low':p('M5 17H53V47H5ZM53 27H59V37H53M12 24H18V40H12Z'),
 'ui-sd-card':p('M18 5H51V59H12V17ZM23 5V19H44V5M20 34H43M20 44H43')
};
const codes={0:['clear-day','clear-night'],1:['mainly-clear-day','mainly-clear-night'],2:['partly-cloudy-day','partly-cloudy-night'],3:['overcast'],45:['fog'],48:['rime-fog'],51:['drizzle'],53:['drizzle'],55:['drizzle'],56:['freezing-drizzle'],57:['freezing-drizzle'],61:['rain'],63:['rain'],65:['heavy-rain'],66:['freezing-rain'],67:['freezing-rain'],71:['snow'],73:['snow'],75:['heavy-snow'],77:['snow-grains'],80:['rain-showers-day','rain-showers-night'],81:['rain-showers-day','rain-showers-night'],82:['heavy-rain-showers'],85:['snow-showers'],86:['snow-showers'],95:['thunderstorm'],96:['hail-thunderstorm'],97:['heavy-thunderstorm'],99:['hail-thunderstorm']};
fs.mkdirSync(path.join(out,'svg'),{recursive:true});
fs.writeFileSync(path.join(out,'weather-codes.json'),JSON.stringify({source:'https://open-meteo.com/en/docs#weathervariables',checked:'2026-10-27',fallback:'unknown',notes:'Entry arrays are [day, night]; single entries apply to both. Daily outlook uses day artwork. Intensity may share artwork; preserve the exact code in text/details.',codes},null,2)+'\n');
for(const [name,art]of Object.entries(icons))fs.writeFileSync(path.join(out,'svg',name+'.svg'),`<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="64" height="64" role="img" aria-label="${name.replaceAll('-',' ')}"><title>${name.replaceAll('-',' ')}</title><g transform="translate(4 4) scale(.875)" fill="none" stroke="#000" stroke-width="2.6" stroke-linecap="round" stroke-linejoin="round">${art}</g></svg>\n`);
(async()=>{
 const browser=await chromium.launch({headless:true,...(process.env.CPW_BROWSER_PATH?{executablePath:process.env.CPW_BROWSER_PATH}:{})});
 const page=await browser.newPage({viewport:{width:64,height:64},deviceScaleFactor:1});
 const sizes=[16,24,32,48,64];
 for(const size of sizes){fs.mkdirSync(path.join(out,'raster',String(size)),{recursive:true});await page.setViewportSize({width:size,height:size});for(const name of Object.keys(icons)){
  const stroke={16:4,24:3.4,32:3,48:2.8,64:2.6}[size];
  const svg=fs.readFileSync(path.join(out,'svg',name+'.svg'),'utf8').replace('stroke-width="2.6"',`stroke-width="${stroke}"`);
  await page.setContent(`<style>html,body{margin:0;background:white}svg{display:block;width:${size}px;height:${size}px}</style>${svg}`);
  await page.screenshot({path:path.join(out,'raster',String(size),name+'.png')});
 }}
 await browser.close();console.log(`Rendered ${Object.keys(icons).length} original icons at ${sizes.length} sizes.`);
})().catch(e=>{console.error(e);process.exit(1)});
