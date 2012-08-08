class Users::RegistrationsController < Devise::RegistrationsController

  def create
    puts'###################################',session[:return_to]
    build_resource

    #resource.confirmation_token=nil
      #resource.confirmed_at=Time.now.utc
      #resource.registration_status=true
    if resource.save
      if resource.active_for_authentication?
        set_flash_message :notice, :signed_up if is_navigational_format?
        sign_in(resource_name, resource)
        #respond_with resource, :location => after_sign_up_path_for(resource)
        if session[:return_to].nil?
          puts'###################################',session[:return_to]
          redirect_to users_dashboard_index_path
        else
          puts'ssssssssssssssssssssssssssssssss',session[:return_to]
          redirect_to home_add_to_cart_path
        end
      else
        set_flash_message :notice, :"signed_up_but_#{resource.inactive_message}" if is_navigational_format?
        expire_session_data_after_sign_in!
        respond_with resource, :location => after_inactive_sign_up_path_for(resource)
      end
    else
      clean_up_passwords resource
      respond_with resource
    end
  end

end