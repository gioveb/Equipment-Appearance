// Equipment Appearance 0.0.10 - native-style mouse hit region, eye only, name anchor and native floating tooltip.
_root.EA_UIIdentity="equipment_appearance:helmet_button:v1";
_root.EA_HideHelmetTooltip=function(){ExternalCommands.SetValueUnicode("GUITooltips.TooltipText","");};
_root.EA_ShowHelmetTooltip=function(b){
 var r=b.EA_Hit.getBounds(_root);
 ExternalCommands.SetValue("GUITooltips.TooltipType",1);
 ExternalCommands.SetValue("GUITooltips.HostAnchor",6);
 ExternalCommands.SetValue("GUITooltips.TooltipAnchor",8);
 ExternalCommands.SetValue("GUITooltips.MinTooltipWidth",0);
 ExternalCommands.SetValue("GUITooltips.MaxTooltipWidth",0);
 ExternalCommands.SetValue("GUITooltips.HostX",r.xMin-(_root.OriginalStageWidth-1024)/2);
 ExternalCommands.SetValue("GUITooltips.HostY",r.yMin-(_root.OriginalStageHeight-768)/2);
 ExternalCommands.SetValue("GUITooltips.HostWidth",r.xMax-r.xMin);
 ExternalCommands.SetValue("GUITooltips.HostHeight",r.yMax-r.yMin);
 ExternalCommands.SetValueUnicode("GUITooltips.TooltipCompare0","");
 ExternalCommands.SetValueUnicode("GUITooltips.TooltipCompare1","");
 ExternalCommands.SetValueUnicode("GUITooltips.TooltipText","Helmet visibility");
 fscommand("EA_HelmetUITooltip","show");
};
_root.EA_DrawHelmetButton=function(){
 var b=_root.EA_HelmetButton;if(b==undefined)return;
 var art=b.EA_Art;art.clear();var color=b.EA_Hover?0xF0D597:0xC4AA6C;
 art.lineStyle(1.2,color,100);
 art.moveTo(1,7);art.curveTo(10,0,20,7);art.curveTo(10,14,1,7);
 art.beginFill(color,100);art.moveTo(8,7);art.curveTo(8,4.5,10.5,4.5);art.curveTo(13,4.5,13,7);art.curveTo(13,9.5,10.5,9.5);art.curveTo(8,9.5,8,7);art.endFill();
 if(b.EA_State==2){art.lineStyle(1.6,color,100);art.moveTo(2,12);art.lineTo(19,2);}
 b._alpha=b.EA_State>0?100:40;
};
_root.EA_UpdateHelmetButton=function(){
 var bar=_root.QuickbarAndEffectBar_mc.EffectBar_mc;
 var a=bar.WeaponSetToggleButton_mc;var label=bar.PlayerName_mc;
 if(a==undefined || label==undefined || label.TextField==undefined)return;
 var b=_root.EA_HelmetButton;
 if(b==undefined){
  b=bar.createEmptyMovieClip("EA_HelmetButton",bar.getNextHighestDepth());_root.EA_HelmetButton=b;
  b.useHandCursor=true;b.EA_State=0;
  var hit=b.createEmptyMovieClip("EA_Hit",1);hit.beginFill(0,100);hit.moveTo(-3,-3);hit.lineTo(24,-3);hit.lineTo(24,17);hit.lineTo(-3,17);hit.lineTo(-3,-3);hit.endFill();hit._visible=true;hit._alpha=0;hit.hitTestDisable=false;b.HitArea_mc=hit;hit.EA_Host=b;
  b.createEmptyMovieClip("EA_Art",2);MovieUtils.DisableHitTest(b.EA_Art);
  b.onRollOver=function(){this.EA_Hover=true;_root.EA_DrawHelmetButton();var self=this;clearInterval(this.EA_TooltipTimer);this.EA_TooltipTimer=setInterval(function(){clearInterval(self.EA_TooltipTimer);self.EA_TooltipTimer=undefined;if(self.EA_Hover)_root.EA_ShowHelmetTooltip(self);},200);};
  b.onRollOut=b.onDragOut=function(){clearInterval(this.EA_TooltipTimer);this.EA_TooltipTimer=undefined;this.EA_Hover=false;_root.EA_DrawHelmetButton();_root.EA_HideHelmetTooltip();};
  b.onRelease=function(){
   var n=flash.external.ExternalInterface.call("EA_HelmetState");
   if(n==undefined || n<0){fscommand("EA_HelmetUIClickRejected","state unavailable");return;}
   var id=Math.floor(n/4);var state=n-id*4;
   if(state>0)flash.external.ExternalInterface.call("EA_ToggleHelmet",id);
  };
  // Match GUI.Element: visible hit region at alpha zero, callbacks on that region.
  hit.onRollOver=function(){fscommand("EA_HelmetUIPointer","hover");this.EA_Host.onRollOver();};
  hit.onRollOut=function(){this.EA_Host.EA_Pressed=false;this.EA_Host.onRollOut();};
  hit.onDragOut=function(){this.EA_Host.onDragOut();};
  hit.onPress=function(){this.EA_Host.EA_Pressed=true;fscommand("EA_HelmetUIPointer","press");};
  hit.onRelease=function(){var host=this.EA_Host;fscommand("EA_HelmetUIPointer","release");if(host.EA_Pressed){host.EA_Pressed=false;host.onRelease();}};
  hit.onReleaseOutside=function(){this.EA_Host.EA_Pressed=false;this.EA_Host.onRollOut();};
  fscommand("EA_HelmetUIReady","");_root.EA_DrawHelmetButton();
 }
 var field=label.TextField;
 var point={x:field._x+field.textWidth+8,y:field._y+2};
 label.localToGlobal(point);bar.globalToLocal(point);
 b._x=point.x;b._y=point.y;b._visible=a._visible;
 var n=flash.external.ExternalInterface.call("EA_HelmetState");var state=0;
 if(n!=undefined && n>=0){var id=Math.floor(n/4);state=n-id*4;}
 if(b.EA_State!=state){b.EA_State=state;_root.EA_DrawHelmetButton();}
 if(!b._visible && b.EA_Hover){b.EA_Hover=false;_root.EA_HideHelmetTooltip();}
};
if(_root.EA_HelmetInterval!=undefined)clearInterval(_root.EA_HelmetInterval);
_root.EA_HelmetInterval=setInterval(_root.EA_UpdateHelmetButton,200);
