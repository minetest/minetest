#
# Random macros
#

# Not used ATM

MACRO (GETDATETIME RESULT)
    IF (WIN32)
		EXECUTE_PROCESS(COMMAND "cmd" /C echo %date% %time% OUTPUT_VARIABLE ${RESULT})
		string(REGEX REPLACE "\n" "" ${RESULT} "${${RESULT}}")
    ELSEIF(UNIX)
        EXECUTE_PROCESS(COMMAND "date" "+%Y-%m-%d_%H:%M:%S" OUTPUT_VARIABLE ${RESULT})
		string(REGEX REPLACE "\n" "" ${RESULT} "${${RESULT}}")
    ELSE (WIN32)
        MESSAGE(SEND_ERROR "date not implemented")
        SET(${RESULT} "Unknown")
    ENDIF (WIN32)

	string(REGEX REPLACE " " "_" ${RESULT} "${${RESULT}}")
ENDMACRO (GETDATETIME)

