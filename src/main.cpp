/**
  src/main.cpp
  This file is part of chntpw-gui, a graphical frontend to chntpw.
  Copyright (C) 2013 Trevor Bergeron

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
**/

#include "main.h"
#include <gtkmm/application.h>

int main(int argc,char *argv[]){
    Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv, "org.blackcatstudios.chntpw-gui");
    t_chntpw chntpw;

    int ret = app->run(chntpw);

    return ret;
}

// from http://dzone.com/snippets/simple-popen2-implementation
#define READ 0
#define WRITE 1
pid_t popen2(const char *command, int *infp, int *outfp){
    int p_stdin[2], p_stdout[2];
    pid_t pid;
    if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0) return -1;
    pid = fork();
    if (pid < 0)
        return pid;
    else
        if (pid == 0) {
            close(p_stdin[WRITE]);
            dup2(p_stdin[READ], READ);
            close(p_stdout[READ]);
            dup2(p_stdout[WRITE], WRITE); 
            execl("/bin/sh", "sh", "-c", command, NULL);
            perror("execl");
            exit(1);
        }

    if (infp == NULL)
        close(p_stdin[WRITE]);
    else
        *infp = p_stdin[WRITE];
    // The way it was p_stdin[read] in this program is still open
    if (outfp == NULL)
        close(p_stdout[READ]);
    else
        *outfp = p_stdout[READ];
    // as well as p_stdout[write], they're closed in the fork, but not in the original program
    //fix is ez:
    close(p_stdin[READ]); // We only write to the forks input anyway
    close(p_stdout[WRITE]); // and we only read from its output
    return pid;
}

t_chntpw::t_chntpw()
    :
        m_box_everything(Gtk::ORIENTATION_VERTICAL, 5),
        m_box_drive(Gtk::ORIENTATION_HORIZONTAL, 5),
        m_box_user(Gtk::ORIENTATION_HORIZONTAL, 5),
        m_box_password(Gtk::ORIENTATION_HORIZONTAL, 5),
        m_box_buttons(Gtk::ORIENTATION_HORIZONTAL, 5),
        m_btn_go(Gtk::Stock::OK),
        m_btn_exit(Gtk::Stock::QUIT),
        m_refTreeModel_dev(Gtk::ListStore::create(m_cols_dev)),
        m_refTreeModel_user(Gtk::ListStore::create(m_cols_user))
{

    set_title("chntpw-gui");
    set_border_width(10);

    add(m_box_everything);
    m_box_everything.pack_start(m_box_drive, Gtk::PACK_SHRINK);
    m_box_everything.pack_start(m_box_user, Gtk::PACK_SHRINK);
    m_box_everything.pack_start(m_box_password, Gtk::PACK_SHRINK);
    m_box_everything.pack_start(m_box_buttons, Gtk::PACK_SHRINK);
    m_box_everything.show();

    m_box_drive.pack_start(m_lbl_drive, Gtk::PACK_SHRINK);
    m_box_drive.pack_start(m_cmb_drive, Gtk::PACK_SHRINK);
    m_box_drive.show();

    m_box_user.pack_start(m_lbl_user, Gtk::PACK_SHRINK);
    m_box_user.pack_start(m_cmb_user, Gtk::PACK_SHRINK);
    m_box_user.show();

    m_box_password.pack_start(m_lbl_password, Gtk::PACK_SHRINK);
    m_box_password.pack_start(m_psw_password, Gtk::PACK_SHRINK);
    m_box_password.show();

    m_box_buttons.pack_start(m_btn_exit, Gtk::PACK_SHRINK);
    m_box_buttons.pack_start(m_btn_go, Gtk::PACK_SHRINK);
    m_box_buttons.show();

    // Drive hbox
    m_lbl_drive.set_text("Partition: ");
    m_lbl_drive.show();

    // Set up combo boxes
    Gtk::TreeModel::Row dev_row;
    Gtk::TreeModel::Row user_row;

    m_cmb_drive.set_model(m_refTreeModel_dev);
    // Get partitions from libblkid
    blkid_cache *dev_cache = new blkid_cache;
    blkid_dev_iterate *dev_target_iter = new blkid_dev_iterate;
    dev_target = new blkid_dev;

    if(blkid_get_cache(dev_cache, NULL)!=0){
        std::cout<<"Failed to retrieve device cache!\n";
    }
    if(blkid_probe_all(*dev_cache)!=0){
        std::cout<<"Failed to probe devices!\n";
    }
    *dev_target_iter=blkid_dev_iterate_begin(*dev_cache);
    bool canbewin;
    std::cout<<"Getting partition list...\t";
    while(blkid_dev_next(*dev_target_iter, dev_target)==0){
        const char *dev_name = blkid_dev_devname(*dev_target);
        const char *dev_label = blkid_get_tag_value(*dev_cache, "LABEL", dev_name);
        canbewin=(!strcmp(blkid_get_tag_value(*dev_cache,"TYPE",dev_name),"ntfs")&&!(dev_label&&!strcmp(dev_label,"System Reserved")));
//        std::cout<<(canbewin?"Device: ":"Ignore: ")<<dev_name<<' '<<(dev_label?dev_label:"(no label)")<<'\n';
        if(canbewin){
            dev_row = *(m_refTreeModel_dev->append());
            dev_row[m_cols_dev.m_col_dev] = dev_name;
            dev_row[m_cols_dev.m_col_name] = (dev_label?dev_label:"(no label)");
//            m_cmb_drive.set_active(dev_row);
        }
    }
    std::cout<<"done.\n";

    m_cmb_drive.pack_start(m_cols_dev.m_col_dev);
    m_cmb_drive.pack_start(m_cols_dev.m_col_name);
    m_cmb_drive.signal_changed().connect(sigc::mem_fun(*this, &t_chntpw::on_drive_change));
    m_cmb_drive.show();
    //on_drive_change(); // Update for the first one and in case there's only one

    // User hbox
    m_lbl_user.set_text("User: ");
    m_lbl_user.show();

    m_cmb_user.show();

    // Password hbox
    m_lbl_password.set_text("New password: ");
    m_lbl_password.show();
    m_psw_password.show();

    // Buttons
    m_btn_exit.signal_clicked().connect(sigc::mem_fun(*this,&t_chntpw::exit));
    m_btn_exit.show();
    m_btn_go.signal_clicked().connect(sigc::mem_fun(*this,&t_chntpw::go));
    m_btn_go.show();
}
t_chntpw::~t_chntpw(){
    delete dev_target;
}

void t_chntpw::go(){
    Gtk::TreeModel::iterator iter = m_cmb_drive.get_active();
    if(!iter)return;
    Gtk::TreeModel::Row row = *iter;
    if(!row)return;
    Gtk::TreeModel::iterator iter_user = m_cmb_user.get_active();
    if(!iter_user)return;
    Gtk::TreeModel::Row row_user = *iter_user;
    if(!row_user)return;

    Glib::ustring target_device, target_mount;
    target_device = row[m_cols_dev.m_col_dev];
    target_mount = "/mnt";
    target_mount.append(target_device); // makes for /mnt/dev/sda1
    mkdir("/mnt/dev", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // ignore returned condition
    mkdir(target_mount.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // ignore returned condition

    std::cout<<"Mounting "<<target_device<<" ...\t\t";
    int mountstatus = mount(target_device.c_str(), target_mount.c_str(), "ntfs", MS_NOATIME|MS_NOEXEC|MS_NODIRATIME, NULL);
    if(mountstatus!=0){
        std::cout<<"failed!\nMount error "<<errno<<'\n';
        return;
    }

    Glib::ustring s_chntpw_cmd = "chntpw.static ";
    s_chntpw_cmd.append(target_mount);
    s_chntpw_cmd.append("/Windows/System32/config/SAM ");
    s_chntpw_cmd.append(target_mount);
    s_chntpw_cmd.append("/Windows/System32/config/SYSTEM -u ");
    s_chntpw_cmd.append(row_user[m_cols_user.m_col_dev]);
    try{
        std::cout<<"done.\nChanging password...\t\t";
        int infp, outfp;
        char buffer[256] = "";

        if (popen2(/*s_chntpw_cmd.c_str()*/"nc 127.0.0.1 12345", &infp, &outfp) <=0){
            std::cout<<"Couldn't exec chntpw.static! Make sure it's in your path.";
            return;
        }

        std::cout<<"Buffer time\n";
        while(read(outfp, buffer, 256)){
            std::cout<<"b: "<<buffer;
            if(strcmp(buffer, "Do you really wish to disable SYSKEY? (y/n) [n] ")==0)
                write(infp, "n\n", 2);
            //else if(0)
        }



    }catch(int e){}

    std::cout<<"done.\nUnmounting device...\t\t";
    umount(target_mount.c_str());
    std::cout<<"done.\n";
}


void t_chntpw::exit(){
    hide();
}

void t_chntpw::on_drive_change(){
    Gtk::TreeModel::iterator iter = m_cmb_drive.get_active();
    if(!iter)return;
    Gtk::TreeModel::Row row = *iter;
    if(!row)return;

    Glib::ustring target_device, target_mount;
    target_device = row[m_cols_dev.m_col_dev];
    target_mount = "/mnt";
    target_mount.append(target_device); // makes for /mnt/dev/sda1
    mkdir("/mnt/dev", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // ignore returned condition
    mkdir(target_mount.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // ignore returned condition

    std::cout<<"Mounting "<<target_device<<" ...\t\t";
    int mountstatus = mount(target_device.c_str(), target_mount.c_str(), "ntfs", MS_NOATIME|MS_NOEXEC|MS_NODIRATIME, NULL);
    if(mountstatus!=0){
        std::cout<<"failed!\nMount error "<<errno<<'\n';
        return;
    }
    std::cout<<"done.\nReading users...\t\t";
    Glib::ustring s_output, s_chntpw_cmd = "chntpw.static -l ";
    char output[1024]="";
    s_chntpw_cmd.append(target_mount);
    s_chntpw_cmd.append("/Windows/System32/config/SAM"); // should really be case insensitive ugh
    m_cmb_user.set_model(m_refTreeModel_user);
    Gtk::TreeModel::Row newrow;
    try{
        FILE *f_chntpw = popen(s_chntpw_cmd.c_str(), "r");
        if(!f_chntpw)throw -1;
        Glib::ustring::size_type b;
        for(int i=0; fgets(output, 1024, f_chntpw);i++){
            if(i>=12){
                newrow = *(m_refTreeModel_user->append());
                s_output = output;
                b = s_output.find(' ', 9);
                newrow[m_cols_user.m_col_dev] = s_output.substr(9, b-9); // assign to box, not to s_output
                b = s_output.find(' ', 42);
                newrow[m_cols_user.m_col_name] = (s_output.substr(42, b-42).length()?"(admin)":"(not admin)");
            }
        }
        pclose(f_chntpw);
    } catch (int e){}
    m_cmb_user.pack_start(m_cols_user.m_col_dev);
    m_cmb_user.pack_start(m_cols_user.m_col_name);

    std::cout<<"done.\nUnmounting device...\t\t";
    umount(target_mount.c_str());
    std::cout<<"done.\n";
}

/* vim: set ts=4 sw=4 tw=0 et :*/
