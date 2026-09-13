import fs from 'node:fs';
import assert from 'node:assert/strict';
const root=new URL('../../',import.meta.url);
const c=JSON.parse(fs.readFileSync(new URL('firmware/interface-contract.json',root),'utf8'));
const netlist=JSON.parse(fs.readFileSync(new URL(c.evidence,root),'utf8'));
const aliases={SafetyLoop:'SAFETY_MON',HomeSwitch:'HOME',AuxControl:'AUX_CTRL',MramCs:'MRAM_CS_N',CounterCs:'COUNTER_CS_N',SpiMosi:'SPI_MOSI',SpiClock:'SPI_SCK',SpiMiso:'SPI_MISO',RfLearn:'RF_LEARN',RfTxId:'RF_TX_ID',RfModeInd:'RF_MODE_IND',VfdTx:'VFD_TX_3V3',VfdRx:'VFD_RX_3V3',UsbDm:'/MCU_Storage/USB_D-',UsbDp:'/MCU_Storage/USB_D+',StatusLed:'/MCU_Storage/STATUS_LED',HoldToRun:'HOLD_TO_RUN',ServiceUp:'SERVICE_UP',ServiceDown:'SERVICE_DOWN',LightControl:'LIGHT_CTRL',CoupledVfdEnable:'VFD_ENABLE',I2cSda:'I2C_SDA',I2cScl:'I2C_SCL'};
for(let n=0;n<5;n++)aliases['RfD'+n]='RF_D'+n;
for(const [name,pin] of Object.entries(c.pins)){
 const net=netlist.nets.find(n=>n.name===aliases[name]);assert.ok(net,name);
 assert.ok(net.nodes.some(n=>n.ref==='U10'&&n.function.startsWith(`IO${pin}_`)),name);
}
for(const [name,netName] of [['UpperLimit','LIMIT_UP'],['LowerLimit','LIMIT_DOWN'],['ServiceKey','SERVICE_KEY']]){
 const u=c.unresolved[name];assert.equal(u.gpio,null);assert.ok(netlist.nets.find(n=>n.name===netName).nodes.some(n=>n.ref==='U10'&&n.function.startsWith(`IO${u.exportedGpio}_`)));
}
assert.equal(c.deploymentReady,false);
const main=fs.readFileSync(new URL('firmware/src/main.cpp',root),'utf8');
assert.doesNotMatch(main,/attachInterrupt|VfdSerial|PositionUpPulse|PositionDownPulse|ESP\.restart/);
assert.doesNotMatch(main,/digitalWrite\(Pins::CoupledVfdEnable,\s*HIGH/);
const board=JSON.parse(fs.readFileSync(new URL('firmware/boards/elevator-n16r8.json',root),'utf8'));
assert.equal(board.upload.flash_size,'16MB');assert.equal(board.build.arduino.memory_type,'qio_opi');
console.log('PASS contract: exported wiring, explicit unresolved assignments, N16R8, inhibited entry point');
