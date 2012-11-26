<?php
/*
 * This code is part of FusionDirectory (http://www.fusiondirectory.org)
 * Copyright (C) 2012 FusionDirectory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

class dsatabs extends tabs
{
  function __construct($config, $data, $dn, $cat = "", $copied_object = NULL)
  {
    parent::tabs($config, $data, $dn, "dsa", $copied_object);
    $this->addSpecialTabs();
  }

  function save($ignore_account = FALSE)
  {
    $baseobject = $this->getBaseObject();

    /* Check for new 'dn', in order to propagate the
       'dn' to all plugins */
    $new_dn = 'cn='.$baseobject->cn.','.get_ou("dsaRDN").$baseobject->base;

    /* Move group? */
    if ($this->dn != $new_dn) {
      /* Write entry on new 'dn' */
      if ($this->dn != "new") {
        $baseobject->move($this->dn, $new_dn);
      }
      /* Happen to use the new one */
      $this->dn = $new_dn;
    }

    return parent::save();
  }
}

?>