// Read-only checks of exported KiCad netlists and Konnect IPC evidence.
// Does not read or modify KiCad source files.
import fs from 'node:fs';
import assert from 'node:assert/strict';

const [netFile, ipcFile] = process.argv.slice(2);
assert(netFile?.endsWith('.net'), 'Supply an exported .net file');
const tokens = fs.readFileSync(netFile, 'utf8').match(/\(|\)|"(?:\\.|[^"\\])*"|[^\s()]+/g);
let index = 0;
function parse() {
  const token = tokens[index++];
  if (token !== '(') return token.startsWith('"') ? JSON.parse(token) : token;
  const result = [];
  while (tokens[index] !== ')') {
    assert(index < tokens.length, 'Unterminated exported netlist');
    result.push(parse());
  }
  index++;
  return result;
}
const root = parse();
const children = (node, name) => node.filter(x => Array.isArray(x) && x[0] === name);
const field = (node, name) => children(node, name)[0]?.[1];
const nets = children(children(root, 'nets')[0], 'net').map(n => ({
  name: field(n, 'name'),
  nodes: children(n, 'node').map(p => ({ref: field(p, 'ref'), pin: field(p, 'pin'), function: field(p, 'pinfunction')})),
}));
const components = children(children(root, 'components')[0], 'comp');
const component = ref => components.find(c => field(c, 'ref') === ref);
const pinNet = (ref, pin) => nets.find(n => n.nodes.some(p => p.ref === ref && p.pin === pin));
const checks = [];
function check(name, predicate) { assert(predicate, name); checks.push(name); }
const isNet = (ref, pin, name) => pinNet(ref, pin)?.name === name;
check('U10 exact N16R8 variant', field(component('U10'), 'value') === 'ESP32-S3-WROOM-1U-N16R8');
for (const [pin, name] of [['37','LIMIT_UP'], ['36','LIMIT_DOWN'], ['15','SERVICE_KEY'], ['35','VFD_COMMS_ENABLE']]) {
  check(`U10.${pin} = ${name}`, isNet('U10', pin, name));
}
for (const pin of ['28','29','30']) check(`PSRAM pin ${pin} has no external connection`,
  pinNet('U10', pin)?.name.startsWith('unconnected-') && pinNet('U10', pin).nodes.length === 1);
const exactNodes = (name, expected) => {
  const actual = nets.find(n => n.name === name)?.nodes.map(p => `${p.ref}.${p.pin}`).sort();
  check(`${name} exact endpoints`, JSON.stringify(actual) === JSON.stringify(expected.sort()));
};
exactNodes('VFD_COMMS_ENABLE', ['U10.35','U24.8','R24.1']);
exactNodes('SERVICE_KEY', ['U10.15','U43.16','R53.1']);
exactNodes('LIMIT_DOWN', ['U10.36','U42.10','R52.1']);
exactNodes('LIMIT_UP', ['U10.37','R62.2']);
check('LIMIT_UP conditioning preserved through R62', pinNet('R62','1') === pinNet('U42','12') && pinNet('R62','1') === pinNet('R51','1'));
check('R62 is 1k', field(component('R62'),'value') === '1k');
check('SERVICE_KEY pull-up retained', field(component('R53'),'value') === '10k' && isNet('R53','2','+3V3'));
check('OE default pulldown retained', field(component('R24'),'value').startsWith('10k') && isNet('R24','2','GND'));
for (const ref of ['R25','U25','J25','TP26','R59','R60','Q41','F41','D42','J47']) check(`${ref} removed`, !component(ref));
check('No obsolete AUX/RUN/enable nets', !nets.some(n => /AUX_|VFD_RUN_|^VFD_ENABLE$/.test(n.name)));
for (const pin of ['3','4']) check(`J11.${pin} disconnected from former UART`, pinNet('J11',pin)?.name.startsWith('unconnected-') && pinNet('J11',pin).nodes.length === 1);
for (const [pin,name] of [['01','+5V'],['02','GND'],['03','/VFD_Interface/VFD_RX_5V'],['04','/VFD_Interface/VFD_TX_5V']]) check(`J24.${pin} preserved`, isNet('J24',pin,name));
check('TX path preserved', pinNet('U10','10') === pinNet('U24','2') && pinNet('U24','13') === pinNet('J24','03'));
check('RX path preserved', pinNet('U10','11') === pinNet('U24','3') && pinNet('U24','12') === pinNet('J24','04'));
check('Safety monitoring preserved', isNet('U10','39','SAFETY_MON') && isNet('U42','16','SAFETY_MON'));
check('Home monitoring preserved', isNet('U10','38','HOME') && isNet('U42','14','HOME'));
if (ipcFile) {
  const entries = JSON.parse(fs.readFileSync(ipcFile,'utf8'));
  for (const entry of entries.filter(e => e.tool === 'get_component_pads')) {
    check('IPC query succeeded', !entry.isError);
    const data = JSON.parse(entry.result);
    check(`${data.reference} evidence is live IPC`, data.source === 'ipc');
    for (const p of data.pads) {
      const pin = String(p.number ?? p.pad_number);
      const expected = pinNet(data.reference, pin)?.name;
      check(`PCB ${data.reference}.${pin} agrees with export`, p.net === expected);
    }
  }
}
const u10 = nets.flatMap(n => n.nodes.filter(p => p.ref === 'U10').map(p => ({pin:p.pin, function:p.function, net:n.name})))
  .sort((a,b) => a.pin.localeCompare(b.pin, undefined, {numeric:true}));
console.log(JSON.stringify({result:'PASS', checkCount:checks.length, checks, componentCount:components.length, netCount:nets.length, u10},null,2));
