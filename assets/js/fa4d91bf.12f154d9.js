"use strict";(self.webpackChunkwebsite=self.webpackChunkwebsite||[]).push([[802],{7813:(e,t,s)=>{s.r(t),s.d(t,{assets:()=>Z,contentTitle:()=>U,default:()=>k,frontMatter:()=>I,metadata:()=>n,toc:()=>z});const n=JSON.parse('{"id":"installation","title":"Installation","description":"Written by TheGameAce","source":"@site/docs/installation.mdx","sourceDirName":".","slug":"/installation","permalink":"/PalSchema/docs/installation","draft":false,"unlisted":false,"tags":[],"version":"current","sidebarPosition":1,"frontMatter":{"sidebar_position":1},"sidebar":"documentationSidebar","next":{"title":"Resources","permalink":"/PalSchema/docs/resources"}}');var a=s(4848),o=s(8453),r=s(6540),l=s(4164),i=s(5627),d=s(6347),c=s(372),u=s(604),h=s(1861),A=s(8749);function m(e){return r.Children.toArray(e).filter((e=>"\n"!==e)).map((e=>{if(!e||(0,r.isValidElement)(e)&&function(e){const{props:t}=e;return!!t&&"object"==typeof t&&"value"in t}(e))return e;throw new Error(`Docusaurus error: Bad <Tabs> child <${"string"==typeof e.type?e.type:e.type.name}>: all children of the <Tabs> component should be <TabItem>, and every <TabItem> should have a unique "value" prop.`)}))?.filter(Boolean)??[]}function p(e){const{values:t,children:s}=e;return(0,r.useMemo)((()=>{const e=t??function(e){return m(e).map((e=>{let{props:{value:t,label:s,attributes:n,default:a}}=e;return{value:t,label:s,attributes:n,default:a}}))}(s);return function(e){const t=(0,h.XI)(e,((e,t)=>e.value===t.value));if(t.length>0)throw new Error(`Docusaurus error: Duplicate values "${t.map((e=>e.value)).join(", ")}" found in <Tabs>. Every value needs to be unique.`)}(e),e}),[t,s])}function f(e){let{value:t,tabValues:s}=e;return s.some((e=>e.value===t))}function g(e){let{queryString:t=!1,groupId:s}=e;const n=(0,d.W6)(),a=function(e){let{queryString:t=!1,groupId:s}=e;if("string"==typeof t)return t;if(!1===t)return null;if(!0===t&&!s)throw new Error('Docusaurus error: The <Tabs> component groupId prop is required if queryString=true, because this value is used as the search param name. You can also provide an explicit value such as queryString="my-search-param".');return s??null}({queryString:t,groupId:s});return[(0,u.aZ)(a),(0,r.useCallback)((e=>{if(!a)return;const t=new URLSearchParams(n.location.search);t.set(a,e),n.replace({...n.location,search:t.toString()})}),[a,n])]}function b(e){const{defaultValue:t,queryString:s=!1,groupId:n}=e,a=p(e),[o,l]=(0,r.useState)((()=>function(e){let{defaultValue:t,tabValues:s}=e;if(0===s.length)throw new Error("Docusaurus error: the <Tabs> component requires at least one <TabItem> children component");if(t){if(!f({value:t,tabValues:s}))throw new Error(`Docusaurus error: The <Tabs> has a defaultValue "${t}" but none of its children has the corresponding value. Available values are: ${s.map((e=>e.value)).join(", ")}. If you intend to show no default tab, use defaultValue={null} instead.`);return t}const n=s.find((e=>e.default))??s[0];if(!n)throw new Error("Unexpected error: 0 tabValues");return n.value}({defaultValue:t,tabValues:a}))),[i,d]=g({queryString:s,groupId:n}),[u,h]=function(e){let{groupId:t}=e;const s=function(e){return e?`docusaurus.tab.${e}`:null}(t),[n,a]=(0,A.Dv)(s);return[n,(0,r.useCallback)((e=>{s&&a.set(e)}),[s,a])]}({groupId:n}),m=(()=>{const e=i??u;return f({value:e,tabValues:a})?e:null})();(0,c.A)((()=>{m&&l(m)}),[m]);return{selectedValue:o,selectValue:(0,r.useCallback)((e=>{if(!f({value:e,tabValues:a}))throw new Error(`Can't select invalid tab value=${e}`);l(e),d(e),h(e)}),[d,h,a]),tabValues:a}}var v=s(9136);const x={tabList:"tabList__CuJ",tabItem:"tabItem_LNqP"};function j(e){let{className:t,block:s,selectedValue:n,selectValue:o,tabValues:r}=e;const d=[],{blockElementScrollPositionUntilNextRender:c}=(0,i.a_)(),u=e=>{const t=e.currentTarget,s=d.indexOf(t),a=r[s].value;a!==n&&(c(t),o(a))},h=e=>{let t=null;switch(e.key){case"Enter":u(e);break;case"ArrowRight":{const s=d.indexOf(e.currentTarget)+1;t=d[s]??d[0];break}case"ArrowLeft":{const s=d.indexOf(e.currentTarget)-1;t=d[s]??d[d.length-1];break}}t?.focus()};return(0,a.jsx)("ul",{role:"tablist","aria-orientation":"horizontal",className:(0,l.A)("tabs",{"tabs--block":s},t),children:r.map((e=>{let{value:t,label:s,attributes:o}=e;return(0,a.jsx)("li",{role:"tab",tabIndex:n===t?0:-1,"aria-selected":n===t,ref:e=>{d.push(e)},onKeyDown:h,onClick:u,...o,className:(0,l.A)("tabs__item",x.tabItem,o?.className,{"tabs__item--active":n===t}),children:s??t},t)}))})}function y(e){let{lazy:t,children:s,selectedValue:n}=e;const o=(Array.isArray(s)?s:[s]).filter(Boolean);if(t){const e=o.find((e=>e.props.value===n));return e?(0,r.cloneElement)(e,{className:(0,l.A)("margin-top--md",e.props.className)}):null}return(0,a.jsx)("div",{className:"margin-top--md",children:o.map(((e,t)=>(0,r.cloneElement)(e,{key:t,hidden:e.props.value!==n})))})}function w(e){const t=b(e);return(0,a.jsxs)("div",{className:(0,l.A)("tabs-container",x.tabList),children:[(0,a.jsx)(j,{...t,...e}),(0,a.jsx)(y,{...t,...e})]})}function S(e){const t=(0,v.A)();return(0,a.jsx)(w,{...e,children:m(e.children)},String(t))}const E={tabItem:"tabItem_Ymn6"};function P(e){let{children:t,hidden:s,className:n}=e;return(0,a.jsx)("div",{role:"tabpanel",className:(0,l.A)(E.tabItem,n),hidden:s,children:t})}const I={sidebar_position:1},U="Installation",Z={},z=[{value:"Getting PalSchema",id:"getting-palschema",level:2},{value:"UE4SS - Correct Version + Installation",id:"ue4ss---correct-version--installation",level:2},{value:"UE4SS Settings &amp; Mod Settings",id:"ue4ss-settings--mod-settings",level:2},{value:"Installing PalSchema &amp; Using PalSchema Mods",id:"installing-palschema--using-palschema-mods",level:2}];function B(e){const t={a:"a",code:"code",h1:"h1",h2:"h2",header:"header",img:"img",li:"li",p:"p",strong:"strong",ul:"ul",...(0,o.R)(),...e.components};return(0,a.jsxs)(a.Fragment,{children:[(0,a.jsx)(t.header,{children:(0,a.jsx)(t.h1,{id:"installation",children:"Installation"})}),"\n",(0,a.jsxs)(t.p,{children:["Written by ",(0,a.jsx)(t.a,{href:"https://next.nexusmods.com/profile/TheGameAceReal/mods?gameId=6063",children:"TheGameAce"})]}),"\n",(0,a.jsxs)(t.p,{children:["This guide will get you through the installation & setup process for PalSchema so you can get to making your own mods or enjoy using other's mods created with this new modding tool! If you have any issues with this process, feel free to reach out at the Palworld Modding Community on Discord for further assistance: ",(0,a.jsx)(t.a,{href:"https://discord.gg/r9j9W67xhA",children:"https://discord.gg/r9j9W67xhA"})]}),"\n",(0,a.jsx)(t.h2,{id:"getting-palschema",children:"Getting PalSchema"}),"\n",(0,a.jsxs)(t.p,{children:["The first step in this process will be getting PalSchema, which can be found on GitHub here: ",(0,a.jsx)(t.a,{href:"https://github.com/Okaetsu/PalSchema/releases",children:"https://github.com/Okaetsu/PalSchema/releases"})]}),"\n",(0,a.jsx)(t.p,{children:"Under each release, you'll see an \"Assets\" tab at the bottom that you can click on. It's under this tab that you'll see all downloadable contents. Within here you'll see two PalSchema downloads and two corresponding source code downloads. You'll want one of the former two downloads. Both of these have the same process."}),"\n",(0,a.jsxs)(t.ul,{children:["\n",(0,a.jsxs)(t.li,{children:[(0,a.jsx)(t.strong,{children:"Regular Edition"}),": The first download option in descending order, this version of PalSchema is perfect for regular players who want to use PalSchema mods. Get this version if you don't have any intention of making your own mods via PalSchema."]}),"\n",(0,a.jsxs)(t.li,{children:[(0,a.jsx)(t.strong,{children:"Dev Edition"}),": The second download option in descending order, this version of PalSchema works like the regular version with added features designed for mod developers. If you intend to make your own mods with PalSchema, this is the recommended option."]}),"\n"]}),"\n",(0,a.jsx)(t.h2,{id:"ue4ss---correct-version--installation",children:"UE4SS - Correct Version + Installation"}),"\n",(0,a.jsxs)(t.p,{children:["The next step of this process is making sure you have UE4SS installed properly, and more specifically the right version of it. As of the current writing of this article, PalSchema only supports the latest experimental version of UE4SS (v.3.0.1-253-g154f502), which can be downloaded directly ",(0,a.jsx)(t.a,{href:"https://github.com/UE4SS-RE/RE-UE4SS/releases/download/experimental/UE4SS_v3.0.1-253-g154f502.zip",children:"here"}),"."]}),"\n",(0,a.jsxs)(t.p,{children:["If you already have this version of UE4SS installed, skip to the next ",(0,a.jsx)(t.a,{href:"#ue4ss-settings--mod-settings",children:"step"}),". If not, here's how to set it up:\nFirstly, find your Palworld installation location. This will vary from person to person based on installation choices."]}),"\n",(0,a.jsxs)(S,{defaultValue:"gamepass",values:[{label:"Game Pass",value:"gamepass"},{label:"Steam",value:"steam"}],children:[(0,a.jsxs)(P,{value:"gamepass",children:[(0,a.jsx)(t.p,{children:'For Gamepass users, open up the Xbox app on your computer and go to the Palworld page. In the top right there should be a button with three dots "...". Select this and hit "manage". The below window should pop up, in which you should select the "Files" tab.'}),(0,a.jsx)(t.p,{children:(0,a.jsx)(t.img,{src:"https://i.gyazo.com/ae58f3b0b400855cb2bebac98d8e37d1.png",alt:""})}),(0,a.jsx)(t.p,{children:'Here, you can see the installation location for Palworld on your computer. If you select "Browse", this location will be brought up in your file explorer.'}),(0,a.jsxs)(t.p,{children:["Now that we've gotten to Palworld's installation location, we'll want to navigate through the folders. It should go ",(0,a.jsx)(t.code,{children:"Content -> Pal -> Binaries -> WinGDK"}),". Put the UE4SS zip file you downloaded in here. Select the zip and then right click -> extract all. If you're using the default Windows way of extracting the files, make sure you extract the contents of the zip rather than extracting UE4SS as a folder. I'll include two examples below for correct and incorrect extraction:"]}),(0,a.jsxs)(t.p,{children:[(0,a.jsx)(t.img,{src:s(4347).A+"",width:"637",height:"311"})," ",(0,a.jsx)(t.img,{src:s(3010).A+"",width:"641",height:"293"})]}),(0,a.jsx)(t.p,{children:"Congratulations, you just installed UE4SS for Palworld!"}),(0,a.jsxs)(t.p,{children:[(0,a.jsx)(t.strong,{children:"Note"}),": If you already have UE4SS installed, that's ok. This process will be the same, except you'll want to allow the file dwmapi.dll to override your current one, to direct the game to using the newly installed version instead. You'll also want to move the mods from your current Mods folder under WinGDK, to the Mods folder in the ue4ss folder instead."]})]}),(0,a.jsxs)(P,{value:"steam",children:[(0,a.jsx)(t.p,{children:'For Steam users, open up your Steam library, click on Palworld and then click the gear icon on the right side, then go to "Manage" and "Browse local files".'}),(0,a.jsx)(t.p,{children:(0,a.jsx)(t.img,{src:s(1762).A+"",width:"1396",height:"389"})}),(0,a.jsx)(t.p,{children:(0,a.jsx)(t.img,{src:s(7431).A+"",width:"399",height:"329"})}),(0,a.jsx)(t.p,{children:'If you select "Browse local files", this location will bring you to the root of your Palworld install in file explorer.'}),(0,a.jsxs)(t.p,{children:["Now that we've gotten to Palworld's installation location, we'll want to navigate through the folders. It should go ",(0,a.jsx)(t.code,{children:"Pal -> Binaries -> Win64"}),". Put the UE4SS zip file you downloaded in here. Select the zip and then right click -> extract all. If you're using the default Windows way of extracting the files, make sure you extract the contents of the zip rather than extracting UE4SS as a folder. I'll include two examples below for correct and incorrect extraction:"]}),(0,a.jsxs)(t.p,{children:[(0,a.jsx)(t.img,{src:s(398).A+"",width:"620",height:"517"})," ",(0,a.jsx)(t.img,{src:s(2715).A+"",width:"622",height:"485"})]}),(0,a.jsx)(t.p,{children:"Congratulations, you just installed UE4SS for Palworld!"}),(0,a.jsxs)(t.p,{children:[(0,a.jsx)(t.strong,{children:"Note"}),": If you already have UE4SS installed, that's ok. This process will be the same, except you'll want to allow the file dwmapi.dll to override your current one, to direct the game to using the newly installed version instead. You'll also want to move the mods from your current Mods folder under Win64, to the Mods folder in the ue4ss folder instead."]})]})]}),"\n",(0,a.jsx)(t.h2,{id:"ue4ss-settings--mod-settings",children:"UE4SS Settings & Mod Settings"}),"\n",(0,a.jsxs)(t.p,{children:["Before moving forward, you'll want to make sure that your settings for UE4SS and your Mods folder are correct. For UE4SS, check the UE4SS-settings.ini file.\n",(0,a.jsx)(t.img,{src:"https://i.gyazo.com/db0632ec3346f1264587163082247c72.png",alt:""})]}),"\n",(0,a.jsx)(t.p,{children:"This can be edited with any text editor, such as Windows notepad or Notepad++. In here, the most common two adjustments that have to be made are setting GraphicsAPI to dx11, and setting bUseUObjectArrayCache to false."}),"\n",(0,a.jsx)(t.p,{children:(0,a.jsx)(t.img,{src:s(3506).A+"",width:"1439",height:"167"})}),"\n",(0,a.jsx)(t.p,{children:(0,a.jsx)(t.img,{src:s(6681).A+"",width:"721",height:"152"})}),"\n",(0,a.jsxs)(t.p,{children:["Within your Mods folder under UE4SS, you'll see two files at the bottom: mods.json, and mods.txt.\n",(0,a.jsx)(t.img,{src:"https://i.gyazo.com/2e748d9a93866388b947b2762cf03b6f.png",alt:""})]}),"\n",(0,a.jsx)(t.p,{children:"Both of these files need to be modified the same way. Using mods.txt as an example:"}),"\n",(0,a.jsx)(t.p,{children:(0,a.jsx)(t.img,{src:"https://i.gyazo.com/7e5308a40fe5d58a152f25f6f1978c04.png",alt:""})}),"\n",(0,a.jsx)(t.p,{children:'In mods.txt here, a value of 0 equates to a value of "false" in mods.json, while a value of 1 equates to a value of true. CheatManagerEnablerMod, ConsoleCommandsMod, ConsoleEnablerMod, BPModLoaderMod, and BPML_Genericfunctions should all be set to 1 or "true" across both files.'}),"\n",(0,a.jsx)(t.h2,{id:"installing-palschema--using-palschema-mods",children:"Installing PalSchema & Using PalSchema Mods"}),"\n",(0,a.jsx)(t.p,{children:"Now that UE4SS is ready to go, we're ready for the long and arduous journey to install PalSchema!"}),"\n",(0,a.jsx)(t.p,{children:"..."}),"\n",(0,a.jsx)(t.p,{children:"Just kidding, it's a quick and easy process from here. Simply take the PalSchema zip you downloaded, extract it, and place the PalSchema folder that results under the Mods folder within UE4SS. You've now successfully installed PalSchema!"}),"\n",(0,a.jsxs)(t.p,{children:[(0,a.jsx)(t.img,{src:s(4178).A+"",width:"628",height:"373"})," ",(0,a.jsx)(t.img,{src:s(6442).A+"",width:"184",height:"145"})]}),"\n",(0,a.jsx)(t.p,{children:"If your goal is mod development, there will be other guides to help you get started."}),"\n",(0,a.jsx)(t.p,{children:'If your goal is to just play PalSchema mods, all you have to do to install these mods is navigate into the PalSchema folder, then into "mods". Folders for different mods will go in here. No further work required!'}),"\n",(0,a.jsx)(t.p,{children:(0,a.jsx)(t.img,{src:s(8316).A+"",width:"627",height:"111"})})]})}function k(e={}){const{wrapper:t}={...(0,o.R)(),...e.components};return t?(0,a.jsx)(t,{...e,children:(0,a.jsx)(B,{...e})}):B(e)}},6681:(e,t,s)=>{s.d(t,{A:()=>n});const n=s.p+"assets/images/GraphicsAPI-6d4a264902223cdb271a46618ac08699.png"},3506:(e,t,s)=>{s.d(t,{A:()=>n});const n=s.p+"assets/images/bUseUObjectArrayCache-2b4d2cdba3057541f16188fdbcd45bad.png"},4347:(e,t,s)=>{s.d(t,{A:()=>n});const n=s.p+"assets/images/correct_ue4ss_install-dc4a39a3197c16468d4452ce6cc84641.png"},4178:(e,t,s)=>{s.d(t,{A:()=>n});const n=s.p+"assets/images/palschema_location-b24985f0dc84528a29e44dd25f0bb75a.png"},6442:(e,t,s)=>{s.d(t,{A:()=>n});const n="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAALgAAACRCAIAAAD/8uVuAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsMAAA7DAcdvqGQAAA0hSURBVHhe7Zx9bFRVGsYvaICIbNaAssjHOtQiSjEESRQSXbeFrVAGEmlCZBNAIdB2twKSppD6jy6lsI3B8AdC0wWKG3YxFZZOS22kJYsBE5YYRXQFpLOAWQiISJDdFex0n/Mx0+nMvcMpPVNmyvPjcnvOe8/ce+49z3nf05l32mfYsGEOIbeir/5JSEIoFGIEhUKM6DNnzhxdJMQbLmaJEQw9xAgKhRhBoRAjKBRiBIVCjKBQiBEUCjGCQiFGdFcoW7ZsOXz48AMPPKDryeTRRx89ceJEY2OjrodBH06fPj1r1ixd9wZnOHbsmEnL20B179+SBP2J7sPKlStVe7wQdtUgmqR2uEt0Syi4jeeff15XeoQff/xx8ODB0Q+u5/uQAHSvoKDg4Ycf3r9//6pVqxLPH9zFggULnnvuObQvKSnJycnRB1KSbgkFtwd3gqej67dF//79J0+e3LevaU+uXLmSl5enK44ze/bsCxcuXL9+Xddt09XuKSorK++///5nn31W193IzMz84YcfLl++jHJdXR38orKnJrH3H+3rosvYw51GO0lYHnzwwT179oiXOQ5mD0SjHCk8akxVtXFl4sSJTz75ZG5uruFg7N69+6mnnlJ9wFVmzpy5c+dOdQhE+hl93YjxzTffVJbkdS8exEp1oZiguXfvXnjHdevW6brEtWPPPPOM6n9ETDHDATBSa9euVUa8EC3FKaJe4tUNQ4xuHv1Ys2bNihUr4CSrqqrgSHA/qG7fvl23cJxXXnnl6tWraADeeuutmKpu5MaRI0fOnj07atQow8E4f/78pUuX4EhQVlP26NGj8ojoJ6bypk2bcFGEgEWLFuGBRhuvXbs2cOBAtExe9xR4RPAWH330EcrTp0/HVRBihg8fjv6oBuDrr7+GAsaPH4/BU5rAU21oaPj888+jOwaXhtg6adIkVFEQoogbjphm6mzqDOolqHp1wxCjO3/iiSfwfDdv3qxuCU8N8+DkyZNwmLqF45w6dWrMmDER/cZUE9DW1tbc3Hzjxg2cdsKECdqakHfeeQeOBI+1sLCwvr4eQ67s6CfiIGYqyujbmTNn4N5hRLe3bt0KIxSjglSSuofRUk8JwzN//nxESRjxxGA5ePCg0mg0aDBlyhRouqioCM2gewQsdFIfluCO8LTREveFMm4HxAxHTLNvv/0WgoMx8hKUE3TDBNMpguerll2guLgYD8Lv9+PC6O5jjz2GbmH2ZGRkoKXqPcYpuirP4Q6mKdZx/fr1w8T99NNPtTUhaqZiJg0YMEApoKskqXsYFbWYxTOBw4AFc1etWMeNG4czqGYxoDNY/E6dOlXXDYgeDrgKbfXGpBuJiRUK1lbwmZiIKMO9K/V9+eWXmCvKxQE8AjwI1Us8F4RJrC7V7Fm6dGkgEMBkVS1jqq48/fTTmBPofVNTUygU0taE4FpwJAsXLoRXU9dVqH6qqIRHAx+LKQUj+olYAyNuIXo+Jal70URWrPAWQ4cO1VYJrq5CAFwjAhAugQmAxpHn7EXMcJiQoBuGxAoFz/39999Xbgq3oRw1lIFuQfIwAtfVkHoJgLOB84yp6kZufPLJJ1iIdXUY4EgQy2POrPoJN47rbtiw4fXXX4cFIJCr/gwaNEjdUbK7F0E5vC+++AKd+e6775RRgYiGS6MPOIroCd3g4SNgoUuqb+ikbtoZdZuJhyOGBN0whBluxAjTNQq5y6FQiBEUCjGCQiFGUCjECAqFGEGhECMoFGIEhUKM4JfUiRF8C58YwdBDjKBQiBEUCjGCQiFGUCjECAqFGNFn3LhxKjuQpDgDBw58+eWXt23bdkfGix4lPYBK3njjjYsXL5aWlt7e9y26yT0PPfTQzZs3dY2kJEolDZJgMFhcXHzkyJEeHjUKJdWJqOTAgQOofv/993dEKxRKqtOvX79Lly4dOnRI18NagYBQ0Kbkw8UsMYKLWWIEhUKM6I5QcstbWlrKc3VNAEvN0kTf5CXpSjc9yrlzI0o7SYX0Urobeg7v3DViHp1I76fba5Tglp3fzC2Lk4oMSxLtcGRUyl1aE7aN0cWoWBUxdY5nJBWwsJhtKlsfL5WmsmzJ+o8nRxzOyLnznHJlK20pk0WUR85dIGWRW755yuECYcte79BJpRoWhKKlooY7gnYppZN1HZzbVb7lJH42HfzYOXf4gCjK8ohHoIoxj4yAkjbrF40c5RNHScpgRSgY75pd0ataBJFSZ71wDgW7zmmbAR/Ll0jKmrSNpAaWhOKc3FIOqYT9h2/UyHNngyiM+fWUkcp0K07+6xunI0qRVMOWUJRUwt4DDsaRYaRs1DemHqWprEC/SMDlbIrBz3qIEfY8CunVUCjECAqFGEGhECMoFGIEhUKMoFCIERQKMYJCIUZQKMQICoUYQaEQIygUYgSFQoygUIgRFvJRju0d397e7oh/jtijHFXA7kTwv/NKRb4bSV8sCOWzv2X9bPhcIQ0nJHSBvZBJx/7ymcCkuf/UrUl6YiP0KE/ioRJpV+1IGmNBKFIGCVQSok56AZY8irdKZPWWUsmvDrasznItkJTAhkdJrBIndGudkJTH1q/HnioxcCckDbAVehKoxFsrWatbgoKW1Zna4kK4UTBYna9NpOexFXo8VdIeuomfg+6Lv1B+dWBJa4kPvOpMy9HGOPKX60Y+3+JabSM9j6XQ4+VLnParFz67594Bf1wxXLeMkJU5Olj1thz74xWbmqXNha9OB3Mquay941jxKGLnqhJYbvzvys+HTZz4+H2qcZc5XpEtXM5Ghp47i5U1iqdKxLH29nvu7a/ezu/E8VOtviXL5dhnrS7yDD0SqMVfFRydSb9yx7DhUcR/d5WIPQxqH0vt4pLmnEqxSt3ofOgZevKrRYtgEOuZTRXHtZH0OBY+6zn618cH//IFd5U47Rda//6L0b/CfvJ8+ZdzSHpiyaN4qETa1VHRiKQvFjzKP/4yVq9AxIJE/xRSESXxr4/T59p/2n5TeFocI+kJ/z4KMcLS+yikt0OhECMoFGIEhUKMYHI1MYLJ1cQIG6FHeRIPlfDdtt6BBaFIGSRQCZOrewOWPIq3SmQ1SVJhAnbPYcOjJFYJk6t7BbZ+PfZUSdLcCelRbIWeBCrx0ooMHPk6dbo6vyOLuiOcdCRWR6W3uaRkMwE76dgKPZ4q8U6uBr4lRc6rPp9PZjBtlEWUw4lvHdnXPn/VaJ0465aSzQTs5GMp9Hj5kgTJ1YJg1asya632g2Yn+GFAJrCJskx6zMoc7TR/oAZepF/7MsZKY3xKNhOwk48VjyJ2riqBpbvJ1SYwATv5WFmjeKpEHPNKrr4lx0+1OjkvqIEX6dfSu3inZDMBO6nY8Cjiv7tKxB4Gte8ytYvF0kSuUQPTPvSr5YdbSjYTsJMPk6uJEZY8iodKpF0dFY1I+sLkamIEk6uJEZbeRyG9HQqFGEGhECMoFGKEhcVseXl5CL/nhGJ32P9082ZNTc3Fixd1U5K2WBDKH9asmffSS6qsfiVWvPfergkTJuxvbv7zu+9SK+mOjTfcQiHs20KhtjZsgp/kBuOQIUNysrPnzfutbEjSGBtCke+yBerqAoG6+kCgvj7QUB9QjmXHjh2NjY1O3z6yIUljLAgFixHsZ/r9Ypvpz5MbLC++OOd3vy9etmy57Tx815xqw0Rr5mPfJlY8ihCC8CVhdyK3+oaG+n0N9WgQEp/7pCyJpUNhaWx4FPnJTsSXyG3mjDy15aEBv9nTC7C2mI3yJcKdwJfIrUE00J8ZxhGbEy2n72qdXBKZyLBqotPXxkZSrOPme3yqtUs+tgLnrsxxfEsCaLsajfTZtB+JPhp16bsSSx7FceBFxKYdidxm5E2fITxKSCopjvxqkY4kU6JLnCI93r4lGR9ISyTF2qldLNuIDOxwq0hWtr/KWbKxs1TiT5vgT2Tj3CXNTrAK7RdXVGSXtIqL5ldXOiXZFcc7Hb3bk7ateBQIpV0sSsLrEr3ta2jcB48Ch+LmUUTutJysoDJHZk6DcOZ0JMUaaJeCyR0hnJUdSbqOEH9awz+RLahdXOJUBiETJvPHYsWjCKV0OJI84Uj0Nn0GDnkvZpv1lyyA19AgamDgRAt/lcufzhC6iMPgtKSL2PAo0mF0OJIG4Uj01rhPNHBdzMrc6Ugs8WRshi94+isUsvzTfMoEfNP88oXCGP5KhyL+tO752HoZ0hkZdBCxKpnMH4sNjwIdtDsdXkRsM16YrjfRwH0xK3KnHR0kvFeLtW/rRhszWjs8SrA1Q3w5Q6RTx4aJ+NMm/hPZCHKRxWzlaBGjcMnw1806jt7tyrHwWc9rK1cWFhTW1e3V9c74Z81at3791upqXSfpiQWhLH/ttaLCQl1xY23Fuu1b/6QrJD2xIJRly1eI9azILVDJBbLSydBes22rbk3SEwtCIXcDFhaz5G6AQiFGUCjECAqFGEGhECMoFGIEhUKMoFCIERQKMYJCIUZQKMQICoUY4Dj/B/ZGxZlKJrPKAAAAAElFTkSuQmCC"},8316:(e,t,s)=>{s.d(t,{A:()=>n});const n="data:image/png;base64,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"},7431:(e,t,s)=>{s.d(t,{A:()=>n});const n=s.p+"assets/images/steam_browse_local_files-918b04094a15c92a167f914d828cb29c.png"},398:(e,t,s)=>{s.d(t,{A:()=>n});const n=s.p+"assets/images/steam_correct_ue4ss_install-6aa25ef0d8f723c7e983ff73d132c1ce.png"},1762:(e,t,s)=>{s.d(t,{A:()=>n});const n=s.p+"assets/images/steam_palworld_settings-398784f79bfa56fed87b7f06dd8bee55.png"},2715:(e,t,s)=>{s.d(t,{A:()=>n});const n=s.p+"assets/images/steam_wrong_ue4ss_install-bb751f92e3af13a15084c96110d04ac9.png"},3010:(e,t,s)=>{s.d(t,{A:()=>n});const n=s.p+"assets/images/wrong_ue4ss_install-1c5fc6fcd016b6931fe75dd4524e147b.png"},8453:(e,t,s)=>{s.d(t,{R:()=>r,x:()=>l});var n=s(6540);const a={},o=n.createContext(a);function r(e){const t=n.useContext(o);return n.useMemo((function(){return"function"==typeof e?e(t):{...t,...e}}),[t,e])}function l(e){let t;return t=e.disableParentContext?"function"==typeof e.components?e.components(a):e.components||a:r(e.components),n.createElement(o.Provider,{value:t},e.children)}}}]);