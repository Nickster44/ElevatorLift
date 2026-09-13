// Deterministic host execution of actual page.tsx handlers. Not a browser/React DOM test.
import fs from 'node:fs';
import vm from 'node:vm';
import path from 'node:path';
import {createRequire} from 'node:module';
import {fileURLToPath} from 'node:url';
const dir=path.dirname(fileURLToPath(import.meta.url));
const root=path.resolve(dir,'../../..');
const req=createRequire(path.join(root,'webapp/package.json'));
const ts=req('typescript');
const src=fs.readFileSync(path.join(root,'webapp/app/page.tsx'),'utf8');
const js=ts.transpileModule(src,{compilerOptions:{jsx:ts.JsxEmit.ReactJSX,module:ts.ModuleKind.CommonJS,target:ts.ScriptTarget.ES2022}}).outputText;
let cursor=0,states=[],requests=[],timers=[],effects=[],mode='fail',reply={};
const hooks={
 useState(init){const i=cursor++;if(!(i in states))states[i]=typeof init==='function'?init():init;return [states[i],v=>states[i]=typeof v==='function'?v(states[i]):v];},
 useCallback(f){return f;},useMemo(f){return f;},useEffect(f){effects.push(f);},useRef(v){return {current:v};}
};
// useMemo must evaluate during rendering, exactly as the hook's result.
hooks.useMemo=f=>f();
const jsx=(type,props)=>({type,props});
const exports={};
const fakeWindow={localStorage:{getItem:()=> 'review-test-token',setItem(){}},setTimeout:(f,ms)=>{timers.push({f,ms});return timers.length;},setInterval:()=>1,clearTimeout(){},clearInterval(){}};
vm.runInNewContext(js,{exports,require:n=>n==='react'?hooks:n==='react/jsx-runtime'?{jsx,jsxs:jsx,Fragment:'fragment'}:req(n),window:fakeWindow,Headers,Date,console,fetch:async(p,init)=>{requests.push({p,init});if(mode==='fail')throw Error('simulated disconnect');return {ok:true,headers:new Headers({'content-type':'application/json'}),json:async()=>reply};}});
function render(){cursor=0;effects=[];return exports.default();}
function nodes(n){if(!n||typeof n!=='object')return [];if(Array.isArray(n))return n.flatMap(nodes);return [n,...nodes(n.props?.children)];}
function text(n){if(n===null||n===undefined||typeof n==='boolean')return '';if(Array.isArray(n))return n.map(text).join(' ');if(typeof n!=='object')return String(n);return text(n.props?.children);}
function button(tree,key){return nodes(tree).find(n=>n.type==='button'&&(n.props['aria-label']===key||n.props.className===key));}
const results=[];
function check(ok,name){results.push({confirmed:!!ok,name});console.log(`${ok?'CONFIRMED':'NOT REPRODUCED'} ${name}`);}
let t=render();
check(text(t).includes('Controller online')&&text(t).includes('All interlocks healthy'),'DEFECT initial display claims live healthy before fetching');
await button(t,'Refresh status').props.onClick();t=render();
check(text(t).includes('Preview data')&&text(t).includes('Communicating'),'DEFECT disconnected preview still claims drive communicating');
// Establish genuine-looking live moving state, then lose status connectivity.
const fixture=fs.readFileSync(path.join(dir,'host-results.txt'),'utf8').split(/\r?\n/).find(l=>l.startsWith('STATUS_JSON '));
mode='ok';reply=JSON.parse(fixture.slice('STATUS_JSON '.length));
await button(t,'Refresh status').props.onClick();t=render();
check(states[1].position===12000&&text(t).includes('Controller online'),'actual firmware status JSON is consumed by UI handler');
mode='fail';await button(t,'Refresh status').props.onClick();t=render();
const before=requests.length;await button(t,'control-stop').props.onClick();t=render();
check(requests.length===before&&states[1].state==='idle','DEFECT status loss makes STOP local-only and displays idle');
check(text(t).includes('Car is stationary and ready'),'DEFECT after untransmitted STOP displays stationary ready');
// Local demo arrival timer survives a later live status response.
states[1]={...states[1],state:'idle'};t=render();
const floor=nodes(t).find(n=>n.type==='button'&&!n.props.disabled&&text(n).includes('Landing 3'));
await floor.props.onClick();
const arrival=timers.find(x=>x.ms===1800);
mode='ok';reply={...states[1],state:'moving',position:10000,target:40000};t=render();await button(t,'Refresh status').props.onClick();
arrival.f();t=render();
check(states[3]===false&&states[1].state==='idle'&&states[1].position===40000,'DEFECT pending demo timer overwrites reconnected live motion state');
// Actual UI accepts JSON without validating fields.
reply={};await button(t,'Refresh status').props.onClick();let crashed=false;try{render();}catch{crashed=true;}
check(crashed,'DEFECT malformed status object reaches render and crashes');
const apiPaths=[...src.matchAll(/apiFetch\((?:"|`)([^"`]+)/g)].map(m=>m[1]);
check(apiPaths.length===4&&apiPaths.includes('/api/light/toggle'),'only four actual client API calls (one unsupported light route)');
fs.writeFileSync(path.join(dir,'web-results.json'),JSON.stringify({environment:'simulated hooks/JSX/fetch/timers; unmodified page handlers; no DOM/network',results},null,2));
if(results.some(r=>!r.confirmed))process.exitCode=1;
