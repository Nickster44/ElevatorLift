import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
const dir=path.dirname(fileURLToPath(import.meta.url));
// Read only the Konnect-exported netlist, never KiCad source files.
const tokens=fs.readFileSync(path.join(dir,'current.net'),'utf8').match(/\(|\)|"(?:\\.|[^"\\])*"|[^\s()]+/g);
let i=0;
function parse(){const t=tokens[i++];if(t==='('){const a=[];while(tokens[i]!==')')a.push(parse());i++;return a;}return t.startsWith('"')?JSON.parse(t):t;}
const tree=parse();
const child=(a,k)=>a.find(x=>Array.isArray(x)&&x[0]===k);
const val=(a,k)=>child(a,k)?.[1];
const comps=child(tree,'components').slice(1).map(c=>({ref:val(c,'ref'),value:val(c,'value'),footprint:val(c,'footprint'),sheet:val(child(c,'sheetpath'),'names')}));
const nets=child(tree,'nets').slice(1).map(n=>({name:val(n,'name'),nodes:n.filter(x=>Array.isArray(x)&&x[0]==='node').map(x=>({ref:val(x,'ref'),pin:val(x,'pin'),function:val(x,'pinfunction'),type:val(x,'pintype')}))}));
fs.writeFileSync(path.join(dir,'netlist-summary.json'),JSON.stringify({components:comps,nets},null,2));
for(const ref of ['U10','U11','U12','U13','U14','U20','U21','U22','U23','U24','U25','U30','U31','U32','U33','U40','U41','U42','U43','PS20','PS21','J10','J20','J21','J22','J23','J24','J25','J30','J31','J40','J41','J42','J43','J44','J45','J46','J47']){
 const c=comps.find(c=>c.ref===ref);if(!c)continue;
 console.log('\n'+JSON.stringify(c));
 const pins=nets.flatMap(n=>n.nodes.filter(x=>x.ref===ref).map(x=>`${x.pin} ${x.function??''} = ${n.name}`));console.log(pins.join('\n'));
}
console.log('\nSingle-node full-hierarchy nets: '+JSON.stringify(nets.filter(n=>n.nodes.length===1)));
