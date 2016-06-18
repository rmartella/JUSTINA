;********************************************************
;*							*
;*	schedule_cubes.clp				*
;*							*
;*							*
;*			University of Mexico		*
;*			Julio Cesar Cruz Estrda		*
;*							*
;*			03/02/2016			*
;*							*
;********************************************************
;
; Scene:
;	1) The robot waits for the instruction
;	


(deffacts scheduled_cubes

	(name-scheduled cubes 1 5)
	;;; para iniciar las pruebas con el interprete
	; The robot recieve a command
	; DURATION
	(state (name cubes) (number 1)(duration 6000)(status active))
	; INPUTS
	;(condition (conditional if) (arguments robot zone frontentrance)(true-state 1)(false-state 2)(name-scheduled cubes)(state-number 1))
	; ACTIONS
	(cd-task (cd interp) (actor robot)(obj robot)(from sensors)(to status)(name-scheduled cubes)(state-number 1))

	(tasks false)

	
	
	;(state (name cubes) (number 4)(duration 6000)(status active))
)







