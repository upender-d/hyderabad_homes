module SessionsHelper

  def deny_access
    store_location
    redirect_to new_user_registration_path, :notice => "Need to Register"
  end

   private

    def store_location
      session[:return_to] = request.fullpath
    end

    def clear_stored_location
      session[:return_to] = nil
    end

end