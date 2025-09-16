#include <limits.h>     
#include <vector>
#include <sstream>
#include <iostream>     
#include <unistd.h>     
#include <sys/types.h>  
#include <sys/wait.h>   
#include <sys/stat.h>   
#include <fcntl.h>     
#include <cstring>      
#include <cstdlib>     
#include <pwd.h>       
#include <dirent.h>     
#include <signal.h>
#include <errno.h>
#include <array>
#include <grp.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <iomanip>
#include <algorithm>
#include <ctime> 
#include <fcntl.h> 


// g++ shell.cpp -o shell \
//     -lreadline -lhistory


using namespace std;

string prevDir = "";
pid_t foreground_pid = -1;
pid_t fgPid=-1;
int runCommand(string command,vector<string> token);
vector<string> tokenize(const string &input);
string initialDisplay();



void historyHandler(vector<string> token) {
    // Get total commands stored in history
    int total=history_length;
    if(total == 0) {
        cout<<"No history available"<<endl;
        return;
    }
    int to_show = 10; // default
    if(token.size() == 2) {
        try{
            to_show = stoi(token[1]);
        }
        catch (...)
        {
            cerr<< "history: invalid argument"<<endl;
            return;
        }
        if(to_show > 20) to_show = 20;
        if(to_show<1)
        {
            cerr<< "history: argument must be >= 1"<<endl;
            return;
        }
    }

    if(to_show>total) to_show = total;

    // Get history list
    HIST_ENTRY **the_history = history_list();
    if (!the_history) return;

    // Print last toshow entries
    for (int i = total - to_show; i < total; i++) {
        cout << the_history[i]->line << endl;
    }
}

// autocomplete 
vector<string> all_commands;

void load_system_commands() {
    all_commands.clear();

    // Add built-ins
    all_commands.push_back("ls");
    all_commands.push_back("pinfo");
    all_commands.push_back("search");
    all_commands.push_back("history");
    all_commands.push_back("cd");
    all_commands.push_back("pwd");
    all_commands.push_back("echo");

    // Add system commands from PATH
    const char* path_env = getenv("PATH");
    if(!path_env) return;
    stringstream ss(path_env);
    string dir;
    while(getline(ss, dir, ':')) 
    {
        DIR* dp = opendir(dir.c_str());
        if (!dp) continue;
        struct dirent* entry;
        while((entry = readdir(dp)) != NULL)
        {
            if(entry->d_type == DT_REG || entry->d_type == DT_LNK)
                all_commands.push_back(entry->d_name);
        }
        closedir(dp);
    }

    sort(all_commands.begin(), all_commands.end());
    all_commands.erase(unique(all_commands.begin(), all_commands.end()), all_commands.end());
}

char* command_generator(const char* text, int state) {
    static size_t index;
    static vector<string> matches;

    if(!state){              // first call for this completion
        index = 0;
        matches.clear();
        for(auto &cmd : all_commands){
            if(cmd.rfind(text, 0) == 0)   // prefix match
                matches.push_back(cmd);
        }
    }

    if(index < matches.size())    // return next match
        return strdup(matches[index++].c_str());
    return NULL;                 // no more matches
}

char** my_completion(const char* text, int start, int end) {
    if(start == 0){
        return rl_completion_matches(text, command_generator);
    }
    return NULL; //default filename
}

// end autocomplete

// Load history from file (~/.my_shell_history)
void load_history_from_file() {
    const char* home = getenv("HOME");
    string histfile = string(home ? home : ".") + "/.my_shell_history";
    read_history(histfile.c_str());
}

// Save history to file 
void save_history_to_file() 
{
    const char* home = getenv("HOME");
    string histfile = string(home ? home : ".") + "/.my_shell_history";
    stifle_history(20); // limit history size
    write_history(histfile.c_str());
}
// history end 


// signal start

// void handle_sigint(int sig) {
//     if(fgPid > 0)
//     {
//         kill(fgPid, SIGINT);  
//         cout << "\n"; 
//         initialDisplay();  
//         fflush(stdout);    
//     }
// }

// ctrl+C  working
void sigint_handler(int){
    cout<<"\n";
    rl_on_new_line();
    rl_replace_line("",0);
    // rl_redisplay();
    fflush(stdout);  
}


// Handle CTRL-Z (SIGTSTP) -- recheck
void handle_sigtstp(int sig) {
    if(fgPid > 0){
        kill(fgPid, SIGTSTP);  
        cout << "\nProcess " << fgPid << " stopped and sent to background\n";
        cout << "\n";   
        // initialDisplay();  
        fflush(stdout);    
    }
}


// CTRL-D (EOF)  working
void handle_eof() {
    cout << "\nExiting shell...\n";
    exit(0);
}

// Handle terminated background processes
void handle_sigchld(int sig) {
    int saved_errno = errno; 
    pid_t pid;
    int status;

    // Reap all finished children
    while((pid = waitpid(-1, &status, WNOHANG)) > 0) 
    {
        // cout << "\nBackground process " << pid << " finished";
        if(WIFEXITED(status)){
            // cout << " (exit status: " << WEXITSTATUS(status) << ")";
        }
        cout << endl;
    }
    // errno = saved_errno;  // restore errno
}

//signal end

//1
string initialDisplay()
{
    uid_t userId=getuid();
    struct passwd *pw = getpwuid(userId);  // user info  fetches from /etc/passwd file
    string userName = (pw ? pw->pw_name : "user");

    char systemName[HOST_NAME_MAX];
    gethostname(systemName, sizeof(systemName));  // fetches from kernel memory -> /etc/hostname file on boot

    //Get current working directory 
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd)); 
    // calls kernel -> kernel get info by process PCB -> PCB is maintain by task_struct structure 
    //-> fs_struct.pwd ( kernel updats and maintain it ) PCB have cwd
    
    //Replace homeDirectory with ~
    string homeDirectory = pw->pw_dir;
    string currentWorkingDirectory = cwd;
    if(currentWorkingDirectory.find(homeDirectory)== 0) {
        currentWorkingDirectory.replace(0, homeDirectory.length(), "~");
    }
    return userName + "@" + systemName + ":" + currentWorkingDirectory + ">";
}

//3rd ls
static string exactPath(const string &path_in) {
    if(path_in.empty()) return ".";
    if(path_in[0]=='~')
    {
        uid_t userId=getuid();
        struct passwd *pw = getpwuid(userId);
        string homeDir = pw->pw_dir;

        if(path_in.size()==1){ 
            return homeDir.empty() ? "." : homeDir;
        }
        if(path_in[1] == '/'){
            return (homeDir.empty() ? "" : homeDir)+path_in.substr(1);
        }
    }
    return path_in;
}

static string formatPermissions(mode_t mode) {
    string s;
    if(S_ISDIR(mode)) s+='d';
    else if(S_ISLNK(mode)) s+='l';
    else s += '-';
    s +=(mode & S_IRUSR) ? 'r' : '-';
    s +=(mode & S_IWUSR) ? 'w' : '-';
    s +=(mode & S_IXUSR) ? 'x' : '-';
    s +=(mode & S_IRGRP) ? 'r' : '-';
    s +=(mode & S_IWGRP) ? 'w' : '-';
    s +=(mode & S_IXGRP) ? 'x' : '-';
    s +=(mode & S_IROTH) ? 'r' : '-';
    s +=(mode & S_IWOTH) ? 'w' : '-';
    s +=(mode & S_IXOTH) ? 'x' : '-';
    return s;
}

static void printLong(const string &basePath, const string &fileName) {
    string full = basePath;
    if (!full.empty() && full.back() != '/') 
        full += '/';

    full += fileName;

    struct stat st;
    if(lstat(full.c_str(), &st) < 0) 
    {
        perror(("stat: " + full).c_str());
        return;
    }

    // permissions
    string perm = formatPermissions(st.st_mode);
    // links
    long nlink = st.st_nlink;
    // group, owner
    struct group  *gr = getgrgid(st.st_gid);
    string group = gr ? gr->gr_name : to_string(st.st_gid);

    struct passwd *pw = getpwuid(st.st_uid);
    string owner = pw ? pw->pw_name : to_string(st.st_uid);
    // size
    long long size = st.st_size;

    // time
    char timebuf[64];
    struct tm *mt = localtime(&st.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", mt);

    // name (for symlink append -> target)
    string nameToShow = fileName;
    if(S_ISLNK(st.st_mode)){
        char target[4096];
        ssize_t len = readlink(full.c_str(), target, sizeof(target)-1);
        if(len != -1){
            target[len] = '\0';
            nameToShow += " -> ";
            nameToShow += string(target);
        }
    }

    // permissions, links, owner, group, size, time, name
    cout << perm << ' '
         << setw(2) << nlink << ' '
         << setw(8) << left << owner << ' '
         << setw(8) << left << group << ' '
         << setw(8) << right << size << ' '
         << timebuf << ' '
         << nameToShow
         << '\n';
}

void listPath(string rawPath, bool showHidden, bool detailedInfo)
{
    string path=exactPath(rawPath);
    struct stat st;
    if(stat(path.c_str(), &st)==0 && S_ISDIR(st.st_mode))
    {
        //its directory
        DIR *dir=opendir(path.c_str());
        if (!dir) {
            perror(("opendir:"+path).c_str());
            return;
        }
        vector<string> fileNames;
        struct dirent* entry;
        //store all filename present in folder
        while((entry=readdir(dir)) !=NULL)
        {
            string name=entry->d_name;
            if(!showHidden && name.size()>0 && name[0]=='.')
            {
                //skip hidden file when showHidden=false;
            }
            else{
                fileNames.push_back(name);
            }
        }
        sort(fileNames.begin(), fileNames.end());
        closedir(dir);

        if(detailedInfo)
        {
            long long totalBlocks = 0;
            // total
            for(const string &file : fileNames){
                string full = path;
                if (!full.empty() && full.back() != '/')
                    full += '/';
                full += file;

                struct stat st;
                if (lstat(full.c_str(), &st) == 0){
                    totalBlocks += st.st_blocks;  
                }
            }
            cout << "total " << (totalBlocks / 2) << '\n'; 
            
            // print details
            for(const string &file: fileNames)
                printLong(path, file);
        }
        else{
            for (const string &file : fileNames) {
                if (file.empty() && file[0] == '.') continue;
                cout << file << '\n';
            }
        }
    }
    else{
        //file
        // path might be a file or not exist
        if(stat(path.c_str(), &st) == 0)
        {
            size_t pos = path.find_last_of('/');
            string parent=(pos == string::npos) ? string(".") : path.substr(0, pos==0?1:pos);
            string fname=(pos == string::npos) ? path : path.substr(pos+1);
            if (detailedInfo) printLong(parent, fname);
            else cout << fname << '\n';
        } 
        else 
        {
            perror(("ls: " + rawPath).c_str());
        }
    }
}

void lsCommand(vector<string> input)
{
    int n=input.size();
    vector<string> headDir;
    bool showHidden=false;
    bool detailedInfo=false;
    // input[0]=ls
    if(n>1){
        for(int i=1;i<n;i++)
        {
            if(input[i][0]=='-')
            {
                for(int j=1;j<input[i].size();j++)
                {
                    if(input[i][j]=='a') showHidden=true;
                    if(input[i][j]=='l') detailedInfo=true;
                }
            }
            else if(input[i][0]=='~'){
                uid_t userId=getuid();
                struct passwd *pw = getpwuid(userId);
                string homeDir = pw->pw_dir;
                headDir.push_back(homeDir);
            }
            else{
                headDir.push_back(input[i]);
            }
        }
    }
    
    if(headDir.empty())
    {
        headDir.push_back(".");
    }

    //output on terminal 
    for(int i=0;i<headDir.size();i++)
    {
        if(headDir.size()>1)
            cout<<headDir[i]<<":"<<endl;
        listPath(headDir[i], showHidden, detailedInfo);
        if(i<headDir.size()-1) 
            cout<<endl;
    }

}
// ls end 

//4th fg bg
void fgbg(vector<string> token)
{
    int pid=fork();
    if(pid<0) {
        perror("fork");
        return;
    }
    if(pid==0){
        vector<char*> argv;
        for (auto &s : token) argv.push_back((char*)s.c_str());
        argv.push_back(NULL);
        // initialDisplay();
        execvp(argv[0], argv.data());
        // perror("execvp");
        _exit(127);
    }
    else {
        // parent process
        setpgid(pid, pid); 
        cout << "PID: " << pid << endl;
        // initialDisplay();
        // do not wait (background job continues)
    }
}
   

vector<string> splitPipeline(string input){
    vector<string> commands;
    char *str = strdup(input.c_str());
    char *token = strtok(str, "|");
    while(token != NULL){
        string cmd = token;
        // trim spaces
        cmd.erase(0, cmd.find_first_not_of(" \t"));
        cmd.erase(cmd.find_last_not_of(" \t") + 1);
        commands.push_back(cmd);
        token = strtok(NULL, "|");
    }
    free(str);
    return commands;
}
//8 pipe
void pipeline(string input) {
    // Split into commands by |
    vector<string> commands = splitPipeline(input);
    int n = commands.size();

    // Create N-1 pipes
    vector<array<int,2>> pipes(n-1);
    for(int i=0; i<n-1; i++) {
        if(pipe(pipes[i].data()) < 0) {
            perror("pipe");
            return;
        }
    }

    for(int i=0; i<n; i++) {
        pid_t pid = fork();
        if(pid == 0) {
            // Child process
            // Redirect stdin from previous pipe (if not first)
            if(i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            // Redirect stdout to next pipe (if not last)
            if(i < n-1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Close all pipe fds in child
            for(int j=0; j<n-1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Tokenize this command
            vector<string> tokens = tokenize(commands[i]);

            // Handle redirections only in first/last command
            string infile="", outfile="", appendfile="";
            for(int k=0; k<tokens.size(); k++) {
                if(tokens[k] == "<" && i==0) {
                    infile = tokens[k+1];
                    tokens.erase(tokens.begin()+k, tokens.begin()+k+2);
                    k--;
                }
                else if(tokens[k] == ">" && i==n-1) {
                    outfile = tokens[k+1];
                    tokens.erase(tokens.begin()+k, tokens.begin()+k+2);
                    k--;
                }
                else if(tokens[k] == ">>" && i==n-1) {
                    appendfile = tokens[k+1];
                    tokens.erase(tokens.begin()+k, tokens.begin()+k+2);
                    k--;
                }
            }

            if(!infile.empty()) {
                int fd=open(infile.c_str(), O_RDONLY);
                if(fd<0){ perror("open infile");  }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if(!outfile.empty()) {
                int fd=open(outfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if(fd<0){ perror("open outfile");  }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            if(!appendfile.empty()) {
                int fd=open(appendfile.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
                if(fd<0){ perror("open appendfile");  }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            string cmmd=tokens[0];
            runCommand(cmmd, tokens);
            _exit(0);
        }
        else if(pid<0) {
            perror("fork");
            return;
        }
    }

    // Parent: close all pipe ends
    for(int i=0; i<n-1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all children
    for(int i=0; i<n; i++) {
        wait(NULL);
    }
}
//tokenize
vector<string> tokenize(const string &input)
{
    vector<string> tokens;
    char *cstr = strdup(input.c_str());  // make modifiable copy
    char *token = strtok(cstr," \t");   // split by spaces/tabs

    while (token != NULL) {
        tokens.push_back(token);
        token = strtok(NULL," \t");
    }

    free(cstr);
    return tokens;
}
//2. cd 
void cdHandler(vector<string> token)
{
    //Home Directory
    uid_t userId=getuid();
    struct passwd *pw = getpwuid(userId);
    string homeDir = pw->pw_dir;
    //Current Directory
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    string currentDir=cwd;
    
    if(token.size()==1){
        if (chdir(homeDir.c_str()) == 0) {
            prevDir = currentDir;  
        }
    }
    else{
        string path="";
        for(int i=1;i<token.size();i++)
        {
            if(i>1)path+=" ";
            path+=token[i];
        }
        if(path=="~"){
            chdir(homeDir.c_str());
        }
        else if(path==".."){
            chdir("..");
        }
        else if(path=="."){
            // chdir(cwd);
        }
        else if(path=="-"){
            chdir(prevDir.c_str());
        }
        else{
            if(chdir(path.c_str())!=0){
                perror("Error: Invalid Argument");
            }
        }
        prevDir=currentDir;
    } 
}

//2.echo
void echoHandler(vector<string> token)
{
    for(int i=1;i<token.size();i++)
    {
        cout<<token[i]<<" ";
    }
    cout<<endl;
    return;
}
//2.pwd
void pwdHandler(){
   char cwd[PATH_MAX];
   if(getcwd(cwd, sizeof(cwd))){
        cout<<cwd<<endl;
   } 
   else{
    perror("pwd error");
   }
   return;
}

//5 pinfo
void pinfoHandler(vector<string> token)
{
    pid_t id;
    if(token.size()==1)
    {
        id=getpid();
    }
    else{
        id=(uid_t)stoi(token[1]);
    }
    string pid=to_string(id);
    string path="/proc/"+pid+"/stat";

    // proc/pid/stat -- for processState, memory.
    // ifstream file(path); 
    // if (!file.is_open()) {
    //     // cerr<<"Error: Could not open "<<path<< endl;
    //     cout<<"Invalid Argument"<<endl;
    //     return;
    // }
    // string word;
    // vector<string> fileContent;
    // while(file>>word)
    // {
    //     fileContent.push_back(word);
    // }
    // string processState=fileContent[2];
    // string memoryUsage=fileContent[22];

    // int pgrp=stoi(fileContent[4]);
    // int tpgid=stoi(fileContent[7]);
    // if(pgrp==tpgid){
    //     processState+="+";
    // }
    // //executable path: proc/pid/exe
    // string path2 ="/proc/"+pid+"/exe";
    // ifstream file2(path2);
    // if(!file2.is_open()){
    //     cerr<<"Error: Could not open "<<path<< endl;
    //     return;
    // }

    // open stat file
    int fd = open(path.c_str(), O_RDONLY);
    if(fd == -1) 
    {
        cout << "Invalid Argument" << endl;
        return;
    }

    // read stat content
    char buffer[8192];
    ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytes <= 0) {
        cout << "Invalid Argument" << endl;
        return;
    }
    buffer[bytes] = '\0';

    std::stringstream ss{std::string(buffer)};
    string word;
    vector<string> fileContent;
    while (ss >> word) {
        fileContent.push_back(word);
    }

    string processState = fileContent[2];
    string memoryUsage  = fileContent[22];

    int pgrp  = stoi(fileContent[4]);
    int tpgid = stoi(fileContent[7]);
    if (pgrp == tpgid) {
        processState += "+";
    }

    char exe[4096];
    ssize_t len = readlink(("/proc/"+pid+"/exe").c_str(),exe, sizeof(exe)-1);
    string link;
    if(len!= -1)
    {
        exe[len]='\0';
        link=string(exe);
    } 
    else
    {
        link= "Executable path not found";
    }

    cout<<"ProcessState--"<<processState<<endl;
    cout<<"Memory--"<<memoryUsage<<endl;
    cout<<"Executable Path--"<<link<<endl;
    return ;
}

//6 search
bool searchHandler(string file,string cwd)
{
    DIR *dir = opendir(cwd.c_str());
    if(!dir) return false;   

    struct dirent *folder;
    while((folder = readdir(dir)) != NULL) 
    {
        string name = folder->d_name;
        if(name == "." || name == "..") continue;
        if (name == file){
            closedir(dir);
            return true;
        }
        // if directory, search resursively
        string fullPath = cwd + "/" + name;

        struct stat st;
        if(stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)){
            if(searchHandler(file, fullPath))
            {
                closedir(dir);
                return true;
            }
        }
    }
    closedir(dir);
    return false;
}
//7 input redirection   sort < input.txt >> output.txt
void inputOutputFromFile(string inputFile,string outputFile, string outputAppendFile, vector<string> token)
{
    pid_t pid = fork();
    if(pid==0){
        if(inputFile!=""){
            int fd_input=open(inputFile.c_str(),O_RDONLY);
            if(fd_input<0){
                perror("open");
                return;
            }
            dup2(fd_input, STDIN_FILENO);  // redirect stdin to input file
            close(fd_input);
        }
        if(outputFile!=""){
            int fd_output=open(outputFile.c_str(),O_WRONLY| O_TRUNC |O_CREAT, 0644);
            if(fd_output<0){
                perror("open");
                return;
            }
            dup2(fd_output, STDOUT_FILENO);  // redirect stdout to output file
            close(fd_output);
        }
        if(outputAppendFile!=""){
            int fd_outputAppend=open(outputAppendFile.c_str(),O_WRONLY|O_CREAT |O_APPEND);
            if(fd_outputAppend<0){
                perror("open");
                return;
            }
            dup2(fd_outputAppend, STDOUT_FILENO);  // redirect stdout to output file
            close(fd_outputAppend);
        }

        string cmmd=token[0];
        runCommand(cmmd, token);
        _exit(0);   
    }
    else if(pid>0){
        //parent
        wait(NULL); //wait for child to finish
    }
    else {
        perror("fork");
        // exit(1);
        return ;
    }
}

int runCommand(string command,vector<string> token)
{
     
        if(command=="cd"){
            cdHandler(token);
        }
        else if(command=="echo"){
            echoHandler(token);
        }
        else if (command=="pwd"){
            pwdHandler();
        }
        // 3.
        else if(command=="ls"){
            lsCommand(token);
        }
        //5 pinfo done
        else if(command=="pinfo")
        {
            pinfoHandler(token);
        }
        //6 
        else if(command=="search"){
            if(token.size()>2){
                cout<<"Error: Wrong Input";
                return false;
            }
            if (token.size() < 2) {
                cout << "Error: Missing argument" << endl;
            }
            char cwd[PATH_MAX];
            getcwd(cwd, sizeof(cwd));
            bool found=searchHandler(token[1],cwd);
            if(!found){
                cout<<"False"<<endl;
            }
            else{
                cout<<"True"<<endl;
            }
        }
        else if (command == "history") {
            historyHandler(token);
        }
        else{
            // External command: fork + exec in child; wait in parent; return 1 (handled)
            pid_t pid = fork();
            if (pid == 0) {
                // child
                // Child: restore default signals
                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);

                vector<char*> argv;
                for (auto &s : token) argv.push_back((char*)s.c_str());
                argv.push_back(NULL);
                execvp(argv[0], argv.data());
                perror("execvp");
                _exit(127);
            } else if (pid > 0) {
                fgPid = pid;
                int status;
                waitpid(pid, &status, 0);
                fgPid=-1;
                return 1;
            } else {
                perror("fork");
                return 1;
            }
        }
        return -1;
}

int main()
{
    //  shell_pgid = getpgrp();
    // // Make sure shell is the foreground process group for the terminal
    // tcsetpgrp(STDIN_FILENO, shell_pgid);
    signal(SIGTSTP, SIG_IGN); //
    signal(SIGTTIN,SIG_IGN);
    signal(SIGTTOU,SIG_IGN);
    signal(SIGINT,sigint_handler); //
    // signal(SIGCHLD,sigint_handler);
    signal(SIGCHLD,handle_sigchld);

    // Setup autocomplete
    rl_attempted_completion_function = my_completion;
    
    load_system_commands();
    using_history();
    stifle_history(20);
    load_history_from_file();

    while(true)
    {
        string prompt=initialDisplay(); //1.

        char* line = readline(prompt.c_str());
        if (!line) handle_eof();  // CTRL-D
        if (*line) {
            add_history(line);
            save_history_to_file();
        }
        string input(line);
        free(line);


        // INPUT -> COMMAND ; -> TOKEN \t
        // Split input on ';' to handle multiple commands
        vector<string> commands;
        {
            stringstream ss(input);
            string cmd;
            while (getline(ss, cmd, ';')) {
                // trim leading/trailing spaces
                cmd.erase(0, cmd.find_first_not_of(" \t"));
                cmd.erase(cmd.find_last_not_of(" \t") + 1);
                if (!cmd.empty()) commands.push_back(cmd);
            }
        }

        // Pipeline specific: 
        for (string &cmd : commands) {
        if (cmd.find('|') != string::npos) {
            pipeline(cmd);
            continue;
        }

        vector<string> token=tokenize(cmd);
        if (token.empty()) continue;

        //I/O redirection 
        string inputFile="", outputFile="", outputAppendFile="";
        bool inputFileGiven=false, outputFileGiven=false,outputFileAppendGiven=false;

        for(int i=0;i<token.size();i++)
        {
            if(token[i]=="<"){
                inputFileGiven=true;
                inputFile=token[i+1];
                token.erase(token.begin()+i, token.begin()+i+2);
                i--;
            }
            else if(token[i]==">")
            {
                outputFileGiven=true;
                outputFile=token[i+1];
                token.erase(token.begin()+i, token.begin()+i+2);
                i--;
            }
            else if(token[i]==">>"){
                outputFileAppendGiven=true;
                outputAppendFile=token[i+1];
                token.erase(token.begin()+i, token.begin()+i+2);
                i--;
            }
        }
        
        if(inputFileGiven||outputFileGiven||outputFileAppendGiven)
        {
            inputOutputFromFile(inputFile,outputFile,outputAppendFile, token);
        }
        else if(token.back()=="&") //fg bg
        {
            //its background process;
            token.pop_back();
            fgbg(token);
        }
        else{
            string command= token[0];
            runCommand(command, token);
        }
      }
    }
    return 0;
}