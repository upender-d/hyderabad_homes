class ApplicationController < ActionController::Base
  protect_from_forgery

  include SessionsHelper

  # Customize the Devise after_sign_in_path_for() for redirecct to previous page after login
  def after_sign_in_path_for(resource_or_scope)
    case resource_or_scope
      when :user, User
        store_location = session[:return_to]
        clear_stored_location
        (store_location.nil?) ? "/" : store_location.to_s
      else
        super
    end
  end

   def per_page
    if params[:per_page]
      @per_page = params[:per_page]
    else
      @per_page = 3
    end
  end

  def page
     if params[:page]
     	@page = params[:page]
     else
     	 @page=1
     end
  end

end
