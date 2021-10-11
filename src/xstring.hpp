namespace xeus_wren
{
     
    inline bool startswith(const std::string& str, const std::string& cmp)
    {
      return str.compare(0, cmp.length(), cmp) == 0;
    }
    

    // // f("a.b = f.b")  =  "fb"
    // inline std::string take_(const std::string & code, int cursor_pos){
      
    // }

}