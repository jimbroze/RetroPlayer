class State {
public:
    // Pure virtual function
    virtual State* get_next_state() = 0;
    // print the string
    virtual char* to_string() = 0;
};




class ArduinoController { // Context
public:
    ArduinoController();
    ArduinoController(State* pContext /* Pass Allocated memory */);
    ~ArduinoController();
    // Handles the next state
    void state_changed();
    char* get_state_name();
protected:
    void do_clean_up();
    // Pointer which holds the current state
    // Since this is and base class pointer
    // of Concentrate classes, it can holds their objects
    State* m_pState;
};


class Asleep : public State
{
public:
    virtual State* get_next_state();
    virtual char* to_string();
};