<?php

/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)
  Copyright (C) 2003-2010  Cajus Pollmeier
  Copyright (C) 2011  FusionDirectory

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
*/

class serviceRepository extends goService
{
  /* attribute list for save action */
  //  var $ignore_account         = TRUE;
  var $attributes             = array("FAIrepository");
  var $objectclasses          = array("FAIrepositoryServer");

  /* Repositories */
  var $repositories          = array();
  var $FAIrepository         = array();
  var $conflicts             = array("serviceRepository");
  var $DisplayName           = "";
  var $StatusFlag            = "";

  var $view_logged            = FALSE;
  var $fai_activated          = FALSE;

  var $repositoryMangement = NULL;

  function serviceRepository (&$config, $dn= NULL, $parent= NULL)
  {
    plugin::plugin ($config, $dn,$parent);

    $this->DisplayName = _("Repository service");

    /* Create repositoryMangement */
    $this->repositoryMangement = new repositoryManagement($this->config,$this);

    /* Skip this if fai is deactivated */
    $tmp= $this->config->search("faiManagement", "CLASS",array('menu','tabs'));
    if(!empty($tmp)){
      $this->fai_activated = TRUE;
    }else{
      return;
    }

    $this->repositories = array();
    if(isset($this->attrs['FAIrepository'])){
      for($i = 0; $i < $this->attrs['FAIrepository']['count']; $i++){
        $tmp = explode("|",$this->attrs['FAIrepository'][$i]);
        $repository = array();

        if(isset($tmp[0])) {
          $repository['Url']          = $tmp[0];
        } else {
          $repository['Url']          = "";
        }

        if(!empty($tmp[1])) {
          $repository['ParentServer'] = $tmp[1];
        } else {
          $repository['ParentServer'] = "none";
        }

        if(isset($tmp[2])) {
          $repository['Release']      = $tmp[2];
        } else {
          $repository['Release']      = "";
        }

        if(isset($tmp[3])) {
          $tmp2 = explode(",",$tmp[3]);
          foreach($tmp2 as $sec) {
            $repository['Sections'][$sec]=$sec;
          }
        } else {
          $repository['Section']=array();
        }

        if(isset($tmp[4])) {
          $repository['Install']=$tmp[4];
        } else {
          $repository['Install']="install";
        }

        if(isset($tmp[5])) {
          $repository['Local']=$tmp[5];
        } else {
          $repository['Local']="local";
        }

        if(isset($tmp[6])) {
          $tmp2 = explode(",", $tmp[6]);
          foreach($tmp2 as $arch) {
            $repository['Archs'][$arch] = $arch;
          }
        } else {
          $repository['Archs'] = array();
        }

        $this->repositories[$repository['Release']]=$repository;
      }
    }
    session::set('RepositoryList', $this->repositories);
  }


  /*! \brief    Act on events frowarded by the repositoryManagement
   */
  function newEntry()
  {
    if(!$this->dialog){
      $this->dialog = new serviceRepositorySetup($this->config,$this->dn);
      $this->dialog->parent = $this;
    }
  }


  /*! \brief    Act on events frowarded by the repositoryManagement
   */
  function removeEntry($action,$target,$all)
  {
    foreach($target as $value){

      if(!isset($this->repositories[$value])) continue;

      $url = $this->repositories[$value]['Url'];
      $release = $this->repositories[$value]['Release'];

      $ldap = $this->config->get_ldap_link();
      $ldap->cd ($this->config->current['BASE']);

      $found= false;
      $auto= false;
      $obj= array();
      $aobj= array();

      $ldap->search("(&(objectClass=gotoWorkstation)(objectClass=FAIobject)(|(FAIdebianMirror=".$url.")(FAIdebianMirror=auto)))",array("cn","FAIclass", "FAIdebianMirror"));
      if ($ldap->count() != 0){
        while($attrs = $ldap->fetch()){
          if (!isset($attrs['FAIclass'])){
            continue;
          }
          foreach($attrs['FAIclass'] as $class){
            if(preg_match("/:".str_replace("/","\/",$release)."$/i",$class)){
              if (isset($attrs['FAIdebianMirror'][0]) && $attrs['FAIdebianMirror'][0] == "auto") {
                $aobj[$ldap->getDN()]= $attrs['cn'][0];
                $auto= true;
              } else {
                $obj[$ldap->getDN()]= $attrs['cn'][0];
                $found= true;
              }
            }
          }
        }
      }
      // Look for automatic releases
      if ($auto) {
        $usage= 0;
        $ldap->search("(&(objectClass=FAIrepositoryServer))",array("FAIrepository"));
        if ($ldap->count() != 0){
          while($sattrs = $ldap->fetch()){
            if (!isset($sattrs['FAIrepository'])){
              continue;
            }
            for ($i= 0; $i < $sattrs['FAIrepository']['count']; $i++){
              $t= split('\|', $sattrs['FAIrepository'][$i]);
              if($release == $t[2]){
                $usage++;
              }
            }
          }

          // Are we the last one to provide this release?
          if ($usage < 2) {
            $obj= array_merge($obj, $aobj);
            $found= true;
          }
        }
      }

      if ($found){
        msg_dialog::display(_("Error"), msgPool::stillInUse(_("FAI release"), msgPool::buildList($obj)), ERROR_DIALOG);
      }else{
        if(isset($this->repositories[$value])){
          unset($this->repositories[$value]);
        }
      }
    }
  }


  /*! \brief    Act on events frowarded by the repositoryManagement
   */
  function editEntry($action,$target,$all)
  {
    if(count($target) == 1 && !$this->dialog){
      $value = array_pop($target);
      if(isset($this->repositories[$value])){
        $once = true;
        $obj = $this->repositories[$value];

        /* to be able to detect if this was renamed */
        $obj['initialy_was'] = $obj['Release'];
        $this->dialog = new serviceRepositorySetup($this->config,$this->dn,$obj);
        $this->dialog->parent = $this;
      }
    }
  }

  function execute()
  {
    /* Call parent execute */
    plugin::execute();

    $smarty= get_smarty();
    $this->repositoryMangement->handleActions($this->repositoryMangement->detectPostActions());

    if($this->is_account && !$this->view_logged){
      $this->view_logged = TRUE;
      new log("view","server/".get_class($this),$this->dn);
    }

    if(!$this->fai_activated){
      $str = "<h2>"._("You can't use this plugin until FAI is activated.")."</h2>";
      $display = $smarty->fetch(get_template_path('serviceRepository.tpl', TRUE,dirname(__FILE__)));
      return($str.$display);
    }

    /* Fill templating stuff */
    $smarty->assign("is_createable",$this->acl_is_createable());
    $display= "";


    if(isset($_POST['repository_setup_save']) && is_object($this->dialog)){
      $this->dialog->save_object();
      if(($this->dialog->is_new_name())&&(isset($this->repositories[$this->dialog->GetName()]))){
        msg_dialog::display(_("Error"), msgPool::duplicated(_("Name")), ERROR_DIALOG);
      }else

      if(count($this->dialog->check())!=0){
        foreach($this->dialog->check() as $msg){
          msg_dialog::display(_("Error"), $msg, ERROR_DIALOG);
        }
      }else{
        $obj = $this->dialog->save();
        if($this->dialog->is_new_name()){
          $oldname = $this->dialog->initialy_was;
          $this->repositories[$obj['Release']]=$obj;
          unset($this->repositories[$oldname]);
        }else{
          $this->repositories[$obj['Release']]=$obj;
        }
        $this->dialog = FALSE;
        $this->is_dialog= false;
      }
    }

    if(isset($_POST['repository_setup_cancel'])){
      $this->dialog=FALSE;
      $this->is_dialog = false;
    }

    if(is_object($this->dialog)){
      $this->dialog->save_object();
      $this->is_dialog = true;
      return($this->dialog->execute());
    }

    /*
      Repository setup dialog handling /END
    */
    $link   = "<a href='?plug=".$_GET['plug']."&amp;act=open_repository&amp;id=%s'>%s</a>";
    $edit   = "<input type='image' value='%s' name='edit_%s'   src='images/lists/edit.png'>&nbsp;";

    /* Hide delete icon, if delete is not allowed */
    if($this->acl_is_removeable()){
      $delete = "<input type='image' value='%s' name='delete_%s' src='images/lists/trash.png'>";
    }else{
      $delete = "<img src='images/empty.png' alt='&nbsp;'>";
    }

    session::set('RepositoryList', $this->repositories);
    $str = $this->repositoryMangement->execute();
    $display.= $smarty->fetch(get_template_path('serviceRepository.tpl', TRUE,dirname(__FILE__)));
    return($str.$display);
  }


  /* Save data to object */
  function save_object()
  {
    plugin::save_object();
  }


  /* Check supplied data */
  function check()
  {
    /* Call common method to give check the hook */
    $message= plugin::check();
    return ($message);
  }


  /* Save to LDAP */
  function save()
  {
    if(!$this->fai_activated) return;

    plugin::save();

    $arr = array();
    $releases = array();
    foreach($this->repositories as $servername => $conf){
      if ($conf['ParentServer']=="none") {
        $conf['ParentServer'] = "";
      }

      $arr[] =  $conf['Url']."|".
                $conf['ParentServer']."|".
                $conf['Release']."|".
                join(',',$conf['Sections'])."|".
                $conf['Install']."|".
                $conf['Local']."|".
                join(',',$conf['Archs']);

      $releases[$conf['Release']] = $conf;
    }
    $this->attrs['FAIrepository'] = $arr;

    $ldap = $this->config->get_ldap_link();
    $ldap->cd($this->config->current['BASE']);

    $ldap->cat($this->dn, array('dn'));

    if ($ldap->count()) {
      $ldap->cd($this->dn);
      $this->cleanup();
      $ldap->modify ($this->attrs);
      $this->handle_post_events("modify");
    } else {
        // FIXME : add special entry here if missing
      $ldap->cd ($this->config->current['BASE']);
      $ldap->create_missing_trees($this->dn);
      $ldap->cd($this->dn);
      $ldap->add($this->attrs);
      $this->handle_post_events("add");
    }
    $ldap->cd($this->config->current['BASE']);

    // Check if FAI branch is here
    $fai = get_ou("faiBaseRDN").$this->config->current['BASE'];
    $ldap->cat($fai, array('dn'));
    if(!$ldap->count()){
      $ldap->cd($this->config->current['BASE']);
      $ldap->create_missing_trees($fai);
      if (!$ldap->success()){
        msg_dialog::display(_("LDAP error"), msgPool::ldaperror($ldap->get_error(), $fai, LDAP_ADD, get_class()));
      }
    }

    foreach ($releases as $release => $conf) {
      if ($conf['Install'] == "install") {
        $dn = "ou=$release,$fai";
        $ldap->cat($dn, array('dn'));
        if(!$ldap->count()){
          $ldap->cd ($dn);
          $ldap->add(array('objectClass' => array('organizationalUnit','FAIbranch'), 'ou' => $release));
          if (!$ldap->success()){
            msg_dialog::display(_("LDAP error"), msgPool::ldaperror($ldap->get_error(), $dn, LDAP_ADD, get_class()));
          }
        }
      }
    }

    if($this->initially_was_account){
      new log("modify","server/".get_class($this),$this->dn,array_keys($this->attrs),$ldap->get_error());
    }else{
      new log("create","server/".get_class($this),$this->dn,array_keys($this->attrs),$ldap->get_error());
    }
  }


  function getListEntry()
  {
    $fields = goService::getListEntry();
    $fields['Message']    = _("Repository service");
    return($fields);
  }

  function allow_remove()
  {
    /* Check if the service is still in use
     */
    $ldap = $this->config->get_ldap_link();
    $ldap->cd($this->config->current['BASE']);
    foreach($this->repositories as $repository) {
      $url = $repository['Url'];
      $ldap->search("(&(objectClass=gotoWorkstation)(objectClass=FAIobject)(FAIdebianMirror=".$url."))",array("cn","FAIclass"));
      if ($ldap->count() != 0){
        $obj= array();
        $found= false;
        while($attrs = $ldap->fetch()){
          foreach($attrs['FAIclass'] as $class){
            $obj[$ldap->getDN()]= $attrs['cn'][0];
            $found= true;
          }
        }
        if ($found){
          $message = msgPool::stillInUse(_("Repository service")) . msgPool::buildList($obj);
          return $message;
        }
      }
    }
  }



  function remove_from_parent()
  {
    goService::remove_from_parent();
  }

  /*! \brief    Returns the list of of services, active services.
   *            Used in the filter class for services.
   *             class_filterServerService.inc
   */
  static function getRepositoryList()
  {
    return(session::get('RepositoryList'));
  }

  /* Return plugin informations for acl handling */
  static function plInfo()
  {
    return (array(
          "plShortName"   => _("Repository"),
          "plDescription" => _("Repository service")." ("._("Services").")",
          "plSelfModify"  => FALSE,
          "plDepends"     => array(),
          "plPriority"    => 84,
          "plSection"     => array("administration"),
          "plCategory"    => array("server"),

          "plProvidedAcls"=> array(
#            "start"         => _("Service start"),  // Remove this to hide the start button at all.
#            "stop"          => _("Service stop"),   // Remove this to hide the stop button at all.
#            "restart"       => _("Service restart"),// Remove this to hide the restart button at all.

#            "cn"            => _("Name"),
            "ParentServer"  => _("Parent server"),
            "Release"       => _("Releases"),
            "Url"           => _("URL"),
            "Section"       => _("Sections"),
            "Install"       => _("Installation mirror"),
            "Local"         => _("Local mirror"),
            "Archs"         => _("Architectures"),
          )
        )
      );
  }

}
// vim:tabstop=2:expandtab:shiftwidth=2:filetype=php:syntax:ruler:
?>