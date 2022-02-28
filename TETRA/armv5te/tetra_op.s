.text
.align 4
.code 32
.global div_s
.type div_s, %function

div_s:
	STMFD	sp!,{r4}
	MOV		r2,#0
	MOV		r3,#0x7FFFFFFF
	CMP		r0,#0
	BLT		END
	CMP		r0,r1
	BGT		END
	LSREQ		r2,r3,#16
	BEQ		END

	MOV		r3,r1,LSL #1
	MOV		r4,r1,LSL #2
	

	LSL		r2,r2,#3
	LSL		r0,r0,#3
	
	CMP		r0,r4
	SUBGE	r0,r0,r4
	ORRGE	r2,r2,#4

	CMP		r0,r3
	SUBGE	r0,r0,r3
	ORRGE	r2,r2,#2

	CMP		r0,r1
	SUBGE	r0,r0,r1
	ORRGE	r2,r2,#1
	
	LSL		r2,r2,#3
	LSL		r0,r0,#3
	
	CMP		r0,r4
	SUBGE	r0,r0,r4
	ORRGE	r2,r2,#4

	CMP		r0,r3
	SUBGE	r0,r0,r3
	ORRGE	r2,r2,#2

	CMP		r0,r1
	SUBGE	r0,r0,r1
	ORRGE	r2,r2,#1
	
	LSL		r2,r2,#3
	LSL		r0,r0,#3
	
	CMP		r0,r4
	SUBGE	r0,r0,r4
	ORRGE	r2,r2,#4

	CMP		r0,r3
	SUBGE	r0,r0,r3
	ORRGE	r2,r2,#2

	CMP		r0,r1
	SUBGE	r0,r0,r1
	ORRGE	r2,r2,#1
	
	LSL		r2,r2,#3
	LSL		r0,r0,#3
	
	CMP		r0,r4
	SUBGE	r0,r0,r4
	ORRGE	r2,r2,#4

	CMP		r0,r3
	SUBGE	r0,r0,r3
	ORRGE	r2,r2,#2

	CMP		r0,r1
	SUBGE	r0,r0,r1
	ORRGE	r2,r2,#1
	
	LSL		r2,r2,#3
	LSL		r0,r0,#3
	
	CMP		r0,r4
	SUBGE	r0,r0,r4
	ORRGE	r2,r2,#4

	CMP		r0,r3
	SUBGE	r0,r0,r3
	ORRGE	r2,r2,#2

	CMP		r0,r1
	SUBGE	r0,r0,r1
	ORRGE	r2,r2,#1
	

END:
	MOV		r0,r2
	LDMFD	sp!,{r4}
	BX		lr


