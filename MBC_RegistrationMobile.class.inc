<?php

use DoSomething\MBStatTracker\StatHat;

/**
 * MBC_UserRegistration class - functionality related to the Message Broker
 * consumer mbc-registration-mobile.
 *
 * See:
 * - https://github.com/dosomething/mobilecommons-php
 * - https://mobilecommons.zendesk.com/hc/en-us/articles/202052534-REST-API#ProfileUpdate
 */
class MBC_RegistrationMobile
{

  /**
   * Message Broker connection to RabbitMQ
   */
  private $messageBroker;

  /**
   * Setting from external services - Mailchimp.
   *
   * @var array
   */
  private $settings;

  /**
   * Setting from external services - Mailchimp.
   *
   * @var array
   */
  private $statHat;

  /**
   * Constructor for MBC_TransactionalEmail
   *
   * @param array $settings
   *   Settings from external services - StatHat
   */
  public function __construct($messageBroker, $settings) {

    $this->messageBroker = $messageBroker;
    $this->settings = $settings;

    // Stathat
    $this->statHat = new StatHat($this->settings['stathat_ez_key'], 'mbc-registration-mobile:');
    $this->statHat->setIsProduction(TRUE);
  }

  /**
   * Collect a batch of user registrations for submission to Mobile Commons from
   * the related RabbitMQ queue.
   *
   * @return array $payload
   *   Transactional queue entry details
   */
  public function consumeRegistrationMobileQueue($payload) {

    echo '------- MBC_RegistrationMobile START #' . $payload->delivery_info['delivery_tag'] . ' - ' . date('D M j G:i:s T Y') . ' -------', "\n";

    $payloadDetails = unserialize($payload->body);
    $deliveryTag = $payload->delivery_info['delivery_tag'];

    if ($payloadDetails['activity'] == 'user_welcome-niche' && isset($payloadDetails['mobile'])) {

      // https://secure.mcommons.com/campaigns/5091/opt_in_paths/170071
      $args = array(
        'phone_number' => $payloadDetails['mobile'],
        'opt_in_path_id' => 170071
      );

      if (isset($payloadDetails['email'])) {
        $args['email'] = $payloadDetails['email'];
      }
      if (isset($payloadDetails['zip'])) {
        $args['postal_code'] = $payloadDetails['zip'];
      }
      if (isset($payloadDetails['merge_vars']['FNAME'])) {
        $args['first_name'] = $payloadDetails['merge_vars']['FNAME'];
      }
      if (isset($payloadDetails['merge_vars']['LNAME'])) {
        $args['last_name'] = $payloadDetails['merge_vars']['LNAME'];
      }
      if (isset($payloadDetails['address1'])) {
        $args['street1'] = $payloadDetails['address1'];
      }
      if (isset($payloadDetails['address2'])) {
        $args['street2'] = $payloadDetails['address2'];
      }
      if (isset($payloadDetails['city'])) {
        $args['city'] = $payloadDetails['city'];
      }
      if (isset($payloadDetails['state'])) {
        $args['state'] = $payloadDetails['state'];
      }

      try {
        $config = array(
          'username' => getenv("MOBILE_COMMONS_USER"),
          'password' => getenv("MOBILE_COMMONS_PASSWORD"),
        );
        $MobileCommons = new MobileCommons($config);
        $MobileCommons->profiles_update($args);
        echo '------- MBC_RegistrationMobile mobile: ' . $payloadDetails['mobile'] . ' -------', "\n";

        $this->messageBroker->sendAck($payload);
      }
      catch (Exception $e) {
        trigger_error('mbc-registration-mobile ERROR - Failed to submit "profiles_update" to Mobile Commons API.', E_USER_WARNING);
      }

    }

    echo '------- MBC_RegistrationMobile END #' . $payload->delivery_info['delivery_tag'] . ' - ' . date('D M j G:i:s T Y') . ' -------', "\n";
  }

}