all: manager pump flowmeter fillTank sink monitor

manager: manager.c
	gcc manager.c -o manager
	
pump: pump.c
	gcc pump.c -o pump
	
flowmeter: flowmeter.c
	gcc flowmeter.c -o flowmeter

fillTank: fillTank.c
	gcc fillTank.c -o fillTank
	
sink: sink.c
	gcc sink.c -o sink
	
monitor: monitor.c
	gcc monitor.c -o monitor