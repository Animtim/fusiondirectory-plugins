<?php
/*
          COPYRIGHT

Copyright 2007 Sergio Vaccaro <sergio@inservibile.org>
Copyright 2012 FusionDirectory

This file is part of JSON-RPC PHP.

JSON-RPC PHP is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

JSON-RPC PHP is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with JSON-RPC PHP; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*!
 * \file jsonRPCClient.php
 * Source code for class jsonRPCClient
 */

/*!
 * \brief The object of this class are generic jsonRPC 1.0 clients
 * http://json-rpc.org/wiki/specification
 *
 * \author sergio <jsonrpcphp@inservibile.org>
 */
class jsonRPCClient {

  /*!
   * \brief Debug state
   *
   * \var boolean $debug
   */
  private $debug;

  /*!
   * \brief The server URL
   *
   * !var string $url
   */
  private $url;

  /*!
   * \brief The request id
   *
   * \var integer $id
   */
  private $id;

  /*!
   * \brief If true, notifications are performed instead of requests
   *
   * \var boolean $notification
   */
  private $notification = false;

  /*!
   * \brief HTTP options from http://www.php.net/manual/en/context.http.php
   *
   * \var array $http_options
   */
  private $http_options;

  /*!
   * \brief Takes the connection parameters
   *
   * \param string $url
   *
   * \param array $http_options Additional HTTP options, see http://www.php.net/manual/en/context.http.php
   *
   * \param boolean $debug false
   */
  public function __construct($url, $http_options = array(), $debug = false) {
    // server URL
    $this->url = $url;

    // HTTP options : method, header and content must not be overridden
    unset($http_options['method']);
    unset($http_options['header']);
    unset($http_options['content']);
    $this->http_options = $http_options;
    // debug state
    empty($debug) ? $this->debug = false : $this->debug = true;
    // message id
    $this->id = 1;
  }

  /*!
   * \brief Sets the notification state of the object.
   *        In this state, notifications are performed, instead of requests.
   *
   * \param boolean $notification
   */
  public function setRPCNotification($notification) {
    empty($notification) ?
              $this->notification = false
              :
              $this->notification = true;
  }

  /*!
   * \brief Performs a jsonRCP request and gets the results as an array
   *
   * \param string $method
   *
   * \param array $params
   *
   * \return array
   */
  public function __call($method,$params) {

    $debug="";

    // check
    if (!is_scalar($method)) {
      throw new Exception('Method name has no scalar value');
    }

    // check
    if (is_array($params)) {
      // no keys
      $params = array_values($params);
    } else {
      throw new Exception('Params must be given as array');
    }

    // sets notification or request task
    if ($this->notification) {
      $currentId = NULL;
    } else {
      $currentId = $this->id;
    }

    // prepares the request
    $request = array(
            'method' => $method,
            'params' => $params,
            'id' => $currentId
            );
    $request = json_encode($request);
    $this->debug && $debug.='***** Request *****'."\n".$request."\n".'***** End Of request *****'."\n\n";

    // performs the HTTP POST
    $opts = array (
      'http' => array_merge(
        array (
          'method'  => 'POST',
          'header'  => 'Content-type: application/json',
          'content' => $request
        ),
        $this->http_options
      )
    );
    $context  = stream_context_create($opts);
    if ($fp = @fopen($this->url, 'r', false, $context)) {
      $response = '';
      while($row = fgets($fp)) {
        $response.= trim($row)."\n";
      }
      $this->debug && $debug.='***** Server response *****'."\n".$response.'***** End of server response *****'."\n";
      $response = json_decode($response,true);
    } else {
      throw new Exception('Unable to connect to '.$this->url);
    }

    // debug output
    if ($this->debug) {
      echo nl2br($debug);
    }

    // final checks and return
    if (!$this->notification) {
      // check
      if ($response['id'] != $currentId) {
        throw new Exception('Incorrect response id (request id: '.$currentId.', response id: '.$response['id'].')');
      }
      if (!is_null($response['error'])) {
        throw new Exception('Request error: '.$response['error']);
      }

      return $response['result'];

    } else {
      return true;
    }
  }
}
?>