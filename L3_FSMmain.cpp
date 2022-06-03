#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "L3_timer.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include "mbed.h"


//FSM state -------------------------------------------------
#define L3STATE_IDLE                0
#define L3STATE_CONNECTION          1               //우리가 추가한 state
#define L3STATE_TX                  2               //우리가 추가한 state

//state variables
static uint8_t main_state = L3STATE_IDLE; //protocol state
static uint8_t prev_state = main_state;

//SDU (input)
static uint8_t originalWord[200];
static uint8_t wordLen=0;

static uint8_t sdu[200];

//serial port interface
static Serial pc(USBTX, USBRX);


//application event handler : generating SDU from keyboard input
static void L3service_processInputWord(void)
{
    char input_exit;                                                         ///////esc두번받으려고
    char c = pc.getc();
    if (!L3_event_checkEventFlag(L3_event_dataToSend))
    {
        if (c == '\n' || c == '\r')
        {
            originalWord[wordLen++] = '\0';
            L3_event_setEventFlag(L3_event_dataToSend);
            debug_if(DBGMSG_L3,"word is ready! ::: %s\n", originalWord);
        }
        else
        {
            originalWord[wordLen++] = c;
            if (wordLen >= L3_MAXDATASIZE-1)
            {
                originalWord[wordLen++] = '\0';
                L3_event_setEventFlag(L3_event_dataToSend);
                pc.printf("\n max reached! word forced to be ready :::: %s\n", originalWord);
            }
            else
            {
                if (c == '\t')                                                  ///////TAB입력하면
                {       
                    pc.printf("\n DND MODE ON!\n");
                    main_state = L3STATE_CONNECTION;                            ///////state=cnn이고
                    L3_dnd_timer_startTimer();                                  ///////dnd용 2시간 timer시작한다 
                }
                else if (c == 27)                                              ///////esc입력하면
                {
                    //originalWord[wordLen++] = '\0';                             //////초기화시키고
                    L3_event_setEventFlag(L3_event_MODEctrlRcvd);               /////event 설정(esc두번받으려고)
                    pc.printf("\n ARE YOU SURE EXITING THIS CHAT?\n");
                    main_state = L3STATE_CONNECTION;                            ///////state=CNN이고
                    pc.scanf("%c", &input_exit);                                //////한번더입력받아야됨

                        if(input_exit == 27)                                    //////한번더esc입력되면
                        {
                            pc.printf("\n exit the chatting mode! \n");
                            main_state = L3STATE_IDLE;                          ///////state=IDLE이고
                        }
                        else                                                    //////esc아닌 다른 키 들어오면
                        {
                            pc.printf("\n cancelled exit the chatting mode! \n");
                            main_state = L3STATE_TX;                            ///////state=tx로 돌아온다
                        }
                }
            }
        }
    }
    /*else if(!L3_event_checkEventFlag(L3_event_MODEctrlRcvd))            ///////event = 제어정보 IND
    {
        if (c == '\t')                                                  ///////TAB입력하면
        {
            originalWord[wordLen++] = c;
            pc.printf("\n DND MODE ON!\n");
            main_state = L3STATE_CONNECTION;                            ///////state=cnn이고
            L3_dnd_timer_startTimer();                                  ///////dnd용 2시간 timer시작한다 
        }
         else if (c == 27)                                              ///////esc입력하면
        {
            originalWord[wordLen++] = c;
            pc.printf("\n ARE YOU SURE EXITING THIS CHAT?\n");
            main_state = L3STATE_CONNECTION;                            ///////state=CNN이고
                if(c == 27)                                             //////한번더esc입력되면
                {
                    pc.printf("\n exit the chatting mode! \n");
                    main_state = L3STATE_IDLE;                          ///////state=IDLE이고
                }
                else                                                    //////esc아닌 다른 키 들어오면
                {
                    pc.printf("\n cancelled exit the chatting mode! \n");
                    main_state = L3STATE_TX;                            ///////state=tx로 돌아온다
                }
        }
    }*/
}

//mode 진입을 위한 제어 key SDU 생성=====================================
static void L3service_processInputMode(void)
{ 
    char input_mode = pc.getc();

    pc.printf(":: ENTER THE MODE: \n");      
    
    if(!L3_event_checkEventFlag(L3_event_MODEctrlRcvd))            ///////event = 제어정보 IND
    {                                       
        if(input_mode == '\t')                                      //dnd mode = tab key
        {
            L3_dnd_timer_startTimer();                          //////timer_dnd돌리고 
            main_state = L3STATE_CONNECTION;
        }
        else if(input_mode == 27)                               //exit mode = esc key
        {
            pc.printf("[ERROR]YOU DON'T CONNECTING WITH ANYONE!");
        }
        else if(input_mode == '\n')                             //connection mode = enter key
        {
            pc.printf(":: Give ID for destination: ");
            //pc.scanf("%d", &input_destId);
            main_state = L3STATE_CONNECTION;
            L3_dnd_timer_startTimer();                          ////////timer_cnn 돌려야됨
                                                                    ///////CONNECT_REQ??????????????????/
        }
    
    }
}
/* uint8_t L3_destId_return()
{
    return destIdnum;
} */

//==============================================================

void L3_initFSM()
{
    //initialize service layer
    pc.attach(&L3service_processInputWord, Serial::RxIrq);

    pc.printf("::Give a word to send : ");                          //////////:: for test
}

void L3_FSMrun(void)
{   
    if (prev_state != main_state)
    {
        debug_if(DBGMSG_L3, "[L3] State transition from %i to %i\n", prev_state, main_state);
        prev_state = main_state;
    }

    //FSM should be implemented here! ---->>>>
    switch (main_state)
    {
        case L3STATE_IDLE: //IDLE state description
            
            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens
            {
                //Retrieving data info.
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t size = L3_LLI_getSize();

                debug("\n -------------------------------------------------\nRCVD MSG : %s (length:%i)\n -------------------------------------------------\n", 
                            dataPtr, size);
                
                pc.printf("Give a word to send : ");
                
                L3_event_clearEventFlag(L3_event_msgRcvd);
            }
            else if (L3_event_checkEventFlag(L3_event_dataToSend)) //if data needs to be sent (keyboard input)
            {
#ifdef ENABLE_CHANGEIDCMD
                if (strncmp((const char*)originalWord, "changeID: ",9) == 0)
                {
                    uint8_t myid = originalWord[9] - '0';
                    debug("[L3] requesting to change to srce id %i\n", myid);
                    L3_LLI_configReqFunc(L2L3_CFGTYPE_SRCID, myid);
                }
                else
#endif
                {
                    //msg header setting
                    strcpy((char*)sdu, (char*)originalWord);
                    L3_LLI_dataReqFunc(sdu, wordLen);

                    debug_if(DBGMSG_L3, "[L3] sending msg....\n");
                }
                
                wordLen = 0;

                pc.printf("Give a word to send : ");

                L3_event_clearEventFlag(L3_event_dataToSend);
            }
            break;

        case L3STATE_CONNECTION :                   ///////////////////state = cnn

        case L3STATE_TX :                           ///////////////////state = tx


        default :
            break;
        
    }
}