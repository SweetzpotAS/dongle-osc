<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
ksmps=32

#define R_PORT # 47120 #

turnon 1000

instr 1000
	SMac init "MAC Address"
	kTimestamp init 0
	kValue init 0
	
	outvalue "mac", SMac;

	Stext sprintf "%i", $R_PORT
	outvalue "rcvPortNum", Stext
	ihandle OSCinit $R_PORT
	kAction OSClisten ihandle, "/breathing", "sii", SMac, kTimestamp, kValue

	if (kAction == 1) then
		outvalue "mac", SMac

		Stimestamp sprintfk "%d", kTimestamp
		outvalue "timestamp", Stimestamp

		Svalue sprintfk "%d", kValue
		outvalue "value", Svalue
	endif
endin

</CsInstruments>
<CsScore>
e 3600
</CsScore>
</CsoundSynthesizer>


<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>2143</x>
 <y>222</y>
 <width>863</width>
 <height>542</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="background">
  <r>125</r>
  <g>162</g>
  <b>160</b>
 </bgcolor>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>mac</objectName>
  <x>163</x>
  <y>18</y>
  <width>175</width>
  <height>35</height>
  <uuid>{ebe15113-3bd4-49d5-8aab-80bdb829f904}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label>F3:13:3E:5F:FE:56</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>20</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>border</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>value</objectName>
  <x>163</x>
  <y>131</y>
  <width>120</width>
  <height>35</height>
  <uuid>{48a13284-569b-4812-bfea-7f9ce5ac3bb6}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label>1757</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>20</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>border</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>28</x>
  <y>21</y>
  <width>80</width>
  <height>35</height>
  <uuid>{89dd7166-2655-4238-9f30-954df1a69e8a}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label>MAC</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>20</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>28</x>
  <y>134</y>
  <width>80</width>
  <height>35</height>
  <uuid>{e02e3389-7b24-4935-ac2c-8d3ac6be9ac6}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label>Value</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>20</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>28</x>
  <y>77</y>
  <width>120</width>
  <height>35</height>
  <uuid>{c46aaea8-7f1b-46de-a150-7a3977760d03}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label>Timestamp</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>20</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>timestamp</objectName>
  <x>163</x>
  <y>74</y>
  <width>80</width>
  <height>35</height>
  <uuid>{9533a540-c59d-48fc-8f78-c6e667e0f81e}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label>0</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>20</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>border</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
</bsbPanel>
<bsbPresets>
</bsbPresets>
<EventPanel name="Events" tempo="60.00000000" loop="8.00000000" x="287" y="1173" width="717" height="322" visible="false" loopStart="0" loopEnd="0">    </EventPanel>
<EventPanel name="" tempo="60.00000000" loop="8.00000000" x="147" y="794" width="717" height="346" visible="true" loopStart="0" loopEnd="0">    </EventPanel>
